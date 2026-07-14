# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

import os
import json
import math
import pandas as pd
import argparse

# Initialize parser
parser = argparse.ArgumentParser()
 
# Adding optional argument

parser.add_argument("-output_folder", "--opf", help = "output folder")
parser.add_argument("-input_folder", "--ipf", help = "input folder")
parser.add_argument("-addr_width", "--aw", help = "address width")
parser.add_argument("-no_cache_lines", "--cl", help = "number of cache lines")
parser.add_argument("-no_of_ways", "--nw", help = "set associativity")
parser.add_argument("-data_width", "--dw", help = "data width")
parser.add_argument("-number_of_blocks", "--nb", help = "number of blocks per cache line")

 
# Read arguments from command line
args = parser.parse_args()

#Directory containing hex files

input_folder_path = args.ipf
output_folder_path = args.opf

#cache paramters

addr_width = int(args.aw)
no_of_cache_lines = int(args.cl)
no_of_ways = int(args.nw)
data_width = int(args.dw)
number_of_blocks = int(args.nb)


#cache line size
cache_line_size = number_of_blocks*data_width

#Number of sets
no_of_sets = no_of_cache_lines

#offset_width
offset_width = math.ceil(math.log(cache_line_size, 2))

#set width
set_width = math.ceil(math.log(no_of_sets, 2))

#Tag width
tag_width = addr_width - set_width - offset_width

def largest_n_tags(df, set_index, n, set_index_bits, result_size):

	#convert set_index to binary
	set_index_binary = bin(set_index)[2:].zfill(set_index_bits)

	#Filter the DataFrame for the given set number
	subset = df[df['SetIndex'] == set_index_binary]
	#return empty dataframe if no entries with that set
	if subset.empty:
		return [], [], 0

	#Find the tag with the largest number of rows in the subset
	largest_tags = subset['Tag'].value_counts().nlargest(n).index.tolist()

	#Find the dataframes to be returned
	sorted_results = []

	num_valid_tags = len(largest_tags)

	for tag in largest_tags:
		#filter the subset for each of the largest tags
		result = subset[subset['Tag'] == tag]
		#Populate the offset_data dict
		offset_data = [0 for offset in range(result_size)]		

		for index, row in result.iterrows():
			offset = row['Offset']
			offset_data[int(offset, 2)] = row['Data']

		#Convert the dict to list
		sorted_data = [offset_data[offset] for offset in range(result_size)]
		
		#Append the sorted_data to the sorted_results list
		sorted_results.append(sorted_data)

	return largest_tags, sorted_results, num_valid_tags

def create_hex_string(largest_tag, sorted_results, rows_data, columns_data, valid, dirty):

	hex_string_tag = ""
	hex_string_data = ""

	for i in range(rows_data):
		hex_string_data_line = ""
		for j in range(columns_data):
			data_str = hex(sorted_results[columns_data*i + j])[2:].zfill(2)
			hex_string_data_line = data_str + hex_string_data_line
		hex_string_data = hex_string_data + hex_string_data_line + "\n"

	tag_binary = str(valid) + str(dirty) + largest_tag
	hex_string_tag = tag_binary + "\n"
 
	return hex_string_tag, hex_string_data


#Initialize a dictionary to store the address-to-data mapping
hex_data_dict = {}

#List all files in the folder
file_names = [f for f in os.listdir(input_folder_path) if f.endswith(".hex")]

for file_name in file_names:
	file_path = os.path.join(input_folder_path, file_name)
	
	with open(file_path, 'r') as file:
		lines = file.readlines()

	current_address = None

	for line in lines:
		line = line.strip()
		if line.startswith('@'):
			current_address = int(line[1:], 16)
		else:
			bytes = line.split()
			for byte in bytes:
				hex_data_dict[current_address] = int(byte, 16)
				current_address+=1

#Save the data in JSON file
output_file = output_folder_path + "/data.json"
with open(output_file, "w") as json_file:
	json.dump(hex_data_dict, json_file, indent=1)


#converting dict in to list of tuples
data_list = [(address, data) for address, data in hex_data_dict.items()]

#create a Dataframe from the list of tuples
df = pd.DataFrame(data_list, columns=['Address', 'Data'])

#convert address to binary
df['Binary_Address'] = df['Address'].apply(lambda x: bin(x)[2:].zfill(addr_width))

#Extract the Tag, SetIndex, and Offset bits
df['Tag'] = df['Binary_Address'].apply(lambda x:x[:tag_width])
df['SetIndex'] = df['Binary_Address'].apply(lambda x:x[tag_width:tag_width + set_width])
df['Offset'] = df['Binary_Address'].apply(lambda x:x[tag_width + set_width:])

df = df.drop(columns=['Binary_Address'])

#initialize dict's
combined_string_tag = {}
combined_string_data = {}

for i in range(no_of_ways):
	combined_string_tag[i] = ""
	combined_string_data[i] = ""

#creating hex strings
for i in range(no_of_sets):
	[largest_tags, sorted_results, num_valid_tags] = largest_n_tags(df, i, no_of_ways, set_width, cache_line_size)

	for j in range(num_valid_tags):
		[hex_string_tag, hex_string_data] = create_hex_string(largest_tags[j], sorted_results[j], number_of_blocks, data_width, 1, 0)
		combined_string_tag[j] = combined_string_tag[j] + hex_string_tag
		combined_string_data[j] = combined_string_data[j] + hex_string_data
	
	for j in range(num_valid_tags, no_of_ways):
		[hex_string_tag, hex_string_data] = create_hex_string('0'*tag_width, [0]*cache_line_size, number_of_blocks, data_width, 0, 0)
		combined_string_tag[j] = combined_string_tag[j] + hex_string_tag
		combined_string_data[j] = combined_string_data[j] + hex_string_data

#writing to file
for i in range(no_of_ways):
		f_data = open(output_folder_path + "/data_" + str(i) + ".hex", "w")
		f_tag = open(output_folder_path + "/tag_" + str(i) + ".hex", "w")	
		f_data.write(combined_string_data[i])
		f_tag.write(combined_string_tag[i])
		f_data.close()
		f_tag.close()	
