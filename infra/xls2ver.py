#!/opt/miniconda3/bin/python3

## Script to auto generate decode logic from the instruction spreadsheet
## Input logic is compressed with espresso optimization before being converted to verilog syntax
##
usage='''
## USAGE:
##    xls2ver.py <yaml file>
##    Inputs:
##      <yaml file> :
##                    input cols
##                    output cols
##                    expanded names
##                    <xlsx file> :
##			  ISA spec for I-ext, F-ext, V-ext with relevant columns for decode
##                        Input Cols: Instruction, Opcode, funct3
##                        Decode Cols: EncType, DestReg, SrcA, SrcB
##    Intermediate Files:
##     - <ColName>.tmp : .tmp file format for espresso optimization
##     - <ColName>.esp : .esp file which holds the espresso optimized tables
##    Output:
##     - autogen_<FileName>.log : Log file with messages per Column decoded
##     - autogen_<FileName>.v : Verilog modules (one per Col decoded) with compressed case statements 
##                       that can be directly instantiated in the main RTL
##
'''
## Todo:
##    - F-Ext - everything
##    - V-Ext - everything
##    - Allow for define headers to auto add as prefix
##    - Derive enable value automatically if bus value is defined
## 
## Contact: Ashok Venkatachar

import pandas as pd
import sys
import re
import os
import argparse
import threading
import yaml
import subprocess
import io
import sharepy
ROOTDIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

sp_getxls = "/tools_risc/common/bin/sp_getxls"
SPFile="https://tenstorrent.sharepoint.com/:x:/r/sites/RiscV/Shared%20Documents/Projects/ASC/RTL/RiscV_Decode.xlsx"
xls_file = ROOTDIR+"/gen/rtl/RiscV_Decode.xlsx"

debug = 0
original_stdout = sys.stdout
esp = "/tools_risc/common/bin/espresso"
#getxls = "/home/avenkatachar/bin/gdrive_download.py"
curlxls = "/home/risc/tools/common/curl_xls.sh"
#curlxlsver = "/home/risc/tools/common/curl_xls_versions.sh"
spyGlobalDisable = "// Allow multiple modules in autogen file\n//spyglass disable_block OneModule-ML\n// Cases can sometimes overlap for specific values, but that is fine\n//spyglass disable_block W398"
spyGlobalEnable = "//spyglass enable_block OneModule-ML\n//spyglass enable_block W398"
spyInstrDisable = "//spyglass disable_block STARC05-2.10.3.2b_sa\n//spyglass disable_block STARC05-2.10.3.2b_sb\n//spyglass disable_block W164c"
spyInstrEnable = "//spyglass enable_block STARC05-2.10.3.2b_sa\n//spyglass enable_block STARC05-2.10.3.2b_sb\n//spyglass enable_block W164c"

tmpPath = ROOTDIR+"/gen/rtl/tmp/"
genPath = ROOTDIR+"/gen/rtl/"
cfgPath = ROOTDIR+"/rtl/gen_files/"
infraPath = ROOTDIR+"/infra/"
commonPath = ROOTDIR+"/rtl/common/"
dvWhisperPath = ROOTDIR+"/gen/dv/whisper/"


def is_nan(x): ## No predefined way to check undefined values
  return (x != x)

def elemExists(x,list): ## Sometimes check element in list doesnt work. Not sure why
  exists=0
  for i in list:
    if (str(i) == str(x)):
      exists=1
  return exists

class yml():
  # Construct routine
  def __init__(self,args):
    print("Class: Yml", "__name__ =", repr(__name__), "method = __init__") if debug else 0
    self.args = args
    ymlFile = os.path.realpath(args.yml)
    # Read YAML file
    print("Reading in YAML file: ",ymlFile)
    with open(ymlFile, 'r', encoding='utf8') as stream:
      self.ctrl = yaml.safe_load(stream)
    di_list = list(self.ctrl['DefaultInputs'].keys())
    for Col in self.ctrl['Cols']:
      try:
        t = self.ctrl['Cols'][Col].keys()
      except:
        self.ctrl['Cols'][Col] = {}

      try:
        m_list = self.ctrl['Cols'][Col]['ModuleName']
      except:
        ## Assuming RTL module name is same as Col name
        self.ctrl['Cols'][Col]['ModuleName'] = Col

      try:
        i_list = di_list + (list(self.ctrl['Cols'][Col]['Inputs'].keys()))
      except:
        ## Keep default input list 
        i_list = di_list
        print("Keeping Default list for module %s" % Col) if debug else 0
      self.ctrl['Cols'][Col]['InputList'] = i_list

      if args.cfg:
        self.ctrl['Version'] = args.cfg
      else:
        self.ctrl['Version'] = 'core_v8'

      ## Debug dump of modified YML
      with open('bugs.yaml','w') as yamlfile:
        yaml.safe_dump(self.ctrl, yamlfile)

## XLS object for storing the instruction spreadsheet
class xls():
  # Construct routine
  def __init__(self, yml):
    print("Class: xls", "__name__ =", repr(__name__), "method = __init__") if debug else 0
    print(yml.args)

    if not yml.args.nodnd:
      try:
        subprocess.run([sp_getxls,SPFile,xls_file])
      except:
        print("Unable to download Xls file")
        print("Please manually download RiscV_Decode.xlsx file (into gen/rtl/) and rerun with -nodnd option")
        exit()

    self.xlsFile = genPath+yml.ctrl['xls']
    self.cfgFile = cfgPath+yml.ctrl['Version']+"_global_defines.h"
    self.defFile = infraPath+"autogen_defines.d"
    self.defs = {}     ## Defines Table
    self.yml = yml.ctrl['Cols']
    ## Read in XLS file as Instruction DataFrame
    xls = pd.read_excel(self.xlsFile, sheet_name=None,dtype=str, engine='openpyxl')
    self.df = pd.concat([xls['I-Ext'],xls['Priv'],xls['Zicsr-Ext'],xls['Zifencei-Ext'],xls['Zb-Ext'],xls['M-Ext'],xls['A-Ext'],xls['F-Ext'],xls['V-Ext'],xls['V-LdSt']], axis=0, ignore_index=True)
    # if yml.ctrl['Verilog'] == 'riscv_imabfv.v':
    #   self.df = pd.concat([xls['I-Ext'],xls['M-Ext'],xls['A-Ext'],xls['Zb-Ext'],xls['F-Ext'],xls['V-Ext'],xls['V-LdSt']], axis=0, ignore_index=True)
    # elif yml.ctrl['Verilog'] == 'riscv_imafv.v':
    #   self.df = pd.concat([xls['I-Ext'],xls['M-Ext'],xls['A-Ext'],xls['F-Ext'],xls['V-Ext'],xls['V-LdSt']], axis=0, ignore_index=True)
    # elif yml.ctrl['Verilog'] == 'riscv_imfv.v':
    #   self.df = pd.concat([xls['I-Ext'],xls['M-Ext'],xls['F-Ext'],xls['V-Ext'],xls['V-LdSt']], axis=0, ignore_index=True)
    # elif yml.ctrl['Verilog'] == 'riscv_imf.v':
    #   self.df = pd.concat([xls['I-Ext'],xls['M-Ext'],xls['F-Ext']], axis=0, ignore_index=True)
    # elif yml.ctrl['Verilog'] == 'riscv_im.v':
    #   self.df = pd.concat([xls['I-Ext'],xls['M-Ext']], axis=0, ignore_index=True)
    # else:
    #   self.df = xls['I-Ext']

    ## Prepare output Verilog file to write
    self.vFileName = commonPath+ yml.ctrl['Verilog']
    print("Preparing file to write ..." + self.vFileName)
    self.vFile = open(self.vFileName, 'w')

    ## Prepare Log file to write
    self.lFileName = re.sub(r'\.sv', r'.log', self.vFileName)
    print("Preparing file to write ..." + self.lFileName)
    self.lFile = open(self.lFileName, 'w')

  # Deconstruct routine
  def __del__(self):
    print("Class: xls", "__name__ =", repr(__name__), "method = __del__") if debug else 0
    # for f in self.df:
    #   os.unlink(f)
    self.vFile.close()
    self.lFile.close()

  # Read in defines file. This eventually replaces vars with real values
  def readDef(self):
    with open(self.cfgFile,'r') as i:
      for m in i.readlines():
        if (is_nan(m) or m == "\n" or m.startswith(' *') or m.startswith('//')):  ## Ignore Empty/comment lines
          continue
        result = re.search(r'define\s+(C_\S+)\s+(\S+)', m) ## Only pull in Global Bin defines (which can override autogen_define.d)
        if (result):
          var = result.group(1)
          define = result.group(2)
          self.defs[var] = define


    with open(self.defFile,'r') as i:
      for m in i.readlines():
        if (is_nan(m) or m == "\n" or m.startswith('//')):  ## Ignore Empty/comment lines
          continue
        result = re.search(r'define\s+(\S+)\s+(\S+)', m)
        var = result.group(1)
        define = result.group(2)
        if ('b' in define): ## Currently dont support Hex, Dec
          define = re.sub(r'\d+\'b', r'', define)
        if ('C_' in define) & (define in self.defs): ## Use Global define value to replace this value
          define = re.sub(r'\d+\'b', r'', self.defs[define])
        self.defs[var] = define

  # Write out Verilog Header information
  def writeVerilogHeader(self):
    sys.stdout = self.vFile
    print('/*************************************************')
    print(' * AUTOGENERATED FILE -- DO NOT EDIT')
    print(' * XLS Source = Sharepoint:sites/Specifications/Hardware Documents/RISCV/AS/RTL/RiscV_Decode.xlsx') 
    ## print(' * XLS Version used = ',self.ver)
    print(' *')
    print(' */')
    print()
    print(spyGlobalDisable)
    sys.stdout = original_stdout

  # Write out Verilog Footer information
  def writeVerilogFooter(self):
    sys.stdout = self.vFile
    print()
    print(spyGlobalEnable)
    sys.stdout = original_stdout

## PLA object for espresso optimization
class pla():
#  def __init__(self, inList, outList, caseHash, optType):
  def __init__(self, col):
    print("Class: Pla", "__name__ =", repr(__name__), "method = __init__") if debug else 0
    self.inList = []   ## Input list
    self.outList = {}  ## Output list
    self.i = []        ## Input list broken down to bits
    self.o = []        ## Output list broken down to bits
    self.inPla = {}    ## Input Pla Table
    self.outPla = {}   ## Espresso Optimized Output Pla Table
    #self.opt = optType
    #self.inPla = caseHash

  # Prepare I/O + PLA table
  def seedPla(self,xls,col):
    print("Class: Pla", "__name__ =", repr(__name__), "method = seedPla") if debug else 0
    ## print(xls.df) if debug else 0
    inst = xls.df['Instruction']
    ignore = xls.df['Not Supported']
    ## Input Signal - Assuming Bus Notation
    for i in xls.yml[col]['InputList']:
      try: ## Bus Notation
        result = re.search(r'(\w+)\[(\d+):(\d+)\]', i)
        inName = result.group(1)
        inUpper = result.group(2)
        inLower = result.group(3)
      except: ## Single bit value
        inName = i
        inUpper = '0'
        inLower = '0'
      inListItem = inName+'['+inUpper+":"+inLower+']'
      if not inListItem in self.inList:
        self.inList.append(inListItem)
        for n in range(int(inUpper), int(inLower)-1, -1):
          self.i.append(inName+r'['+str(n)+r']')
    ## Output Signal
    OutputWidthOverride = 0
    try:
      result = xls.yml[col]['Width']
      if (type(result) == int):
        OutputWidthOverride = result
        inName = xls.yml[col]['ModuleName']
        inUpper = str(result-1)
        inLower = '0'
    except:
      try: ## Bus Notation
        result = re.search(r'(\w+)\[(\d+):(\d+)\]', col)
        inName = result.group(1)
        inUpper = result.group(2)
        inLower = result.group(3)
      except: ## Single bit value
        inName = xls.yml[col]['ModuleName']
        inUpper = '0'
        inLower = '0'
    if col == "Instruction":
      self.outList[inName] = "127:0"
    else:
      self.outList[inName] = inUpper+":"+inLower
    for n in range(int(inUpper), int(inLower)-1, -1):
      self.o.append(inName+r'['+str(n)+r']')

    ## PLA Table as key value pairs
    for ind in xls.df.index:
      ## Ignore empty/comment/header lines or Not supported lines
      if (is_nan(inst[ind]) or str(inst[ind]).startswith("#") or str(inst[ind]).startswith("Instruction") or str(inst[ind]).startswith("nan") or not is_nan(ignore[ind])):
        continue
      print("DEBUG: %s %s %s" % (inst[ind], xls.df['Opcode[6:0]'][ind], xls.df[col][ind])) if debug else 0

      key = ''
      for i in xls.yml[col]['InputList']:
        try: ## Bus Notation
          result = re.search(r'\w\[(\d+):(\d+)\]', i)
          lenKey = int(result.group(1)) - int(result.group(2)) + 1
        except: ## Single bit value
          lenKey = 1

        try:
          for c in xls.yml[col]['Inputs'][i].keys():
            ## if xls.df[c][ind] in xls.yml[col]['Inputs'][i][c]: <<< Not sure why this doesnt work
            if elemExists(xls.df[c][ind],xls.yml[col]['Inputs'][i][c]):
              defkey = xls.df[i][ind]
              if defkey in xls.defs.keys():
                temp = xls.defs[defkey]
                key += temp if (len(temp) == lenKey) else '-'*lenKey  ## SrcA/B/C is not encoded value always
              elif is_nan(defkey):
                key += '-' * lenKey
              else:
                key += str(defkey)
            else:
              key += '-' * lenKey
            print("DEBUG: i=%s, c=%s, colval=%s, condval=%s, array=%s, key=%s" %(i,c,xls.df[i][ind],xls.df[c][ind],xls.yml[col]['Inputs'][i][c],key)) if debug else 0
        except:
          if is_nan(xls.df[i][ind]):
            key += '-' * lenKey
          else:
            defkey = xls.df[i][ind]
            if defkey in xls.defs.keys():
              temp = xls.defs[defkey]
              key += temp if (len(temp) == lenKey) else '-'*lenKey  ## SrcA/B/C is not encoded value always
            elif is_nan(defkey):
              key += '-' * lenKey
            else:
              key += str(defkey)

      val = xls.df[col][ind]
      if (OutputWidthOverride): ## Forced single bit output
          if val in xls.defs.keys():
            new_val = xls.defs[val]
          elif is_nan(val):
            new_val = '0' * OutputWidthOverride
          else: ## Better to force 0 if column is undefined
             new_val = '0' * OutputWidthOverride
      else: ## Use Column as is
        if val in xls.defs.keys(): 
          new_val = xls.defs[val]
        elif is_nan(val):
        #elif str(val).startswith('nan'):
          new_val = '0' * len(self.o)
        else:
          new_val = val

      if key in self.inPla.keys():
        if not (new_val == self.inPla[key]):
          print("WARNING: Conflicting values found for %s, Instr:%s , Key : %s, Values : %s != %s" % (col, inst[ind], key, new_val, self.inPla[key] ), file = xls.lFile)
      else:
        self.inPla[key] = new_val

    # Prepare an array of hashes (One for each of the output elements)
    self.casez = [dict() for x in range(len(self.o))]

  # Print truth table to tFile, and call espresso to minimize it
  # Result stores to plaStr
  def genPla(self,xls,col):
    print("Class: Pla", "__name__ =", repr(__name__), "method = genPla") if debug else 0
    # tFileName = self.ColName.replace(".pla","") + ".tmp"
    tFileName = xls.yml[col]['ModuleName'] + ".tmp"
    self.seedPla(xls,col)
    print("\tGenerating PLA Table ..." + tFileName)
    with open(tmpPath+tFileName, 'w') as tFile:
      sys.stdout = tFile
      print ("# .tmp file: Input format for espresso optimization")
      print ("# .esp file: Output - This is the espresso optimized file\n")
      print (".i %d" % len(self.i))
      print (".o %d\n" % len(self.o))
      print (".ilb %s" % (' '.join(self.i)))
      print (".ob %s\n" % (' '.join(self.o)))
      #print >>tFile, ".type %s\n" % self.opt
      for k in self.inPla.keys():
        ## print("Key = ",k,"Value = ",self.inPla[k])
        inStr = str(k)
        inStr = re.sub(r'_', r'', inStr)
        inStr = re.sub(r'x', r'-', inStr)
        ## outStr= re.sub(r'^0b', r'', self.inPla[k])
        outStr= self.inPla[k]
        outStr= '0' * (len(self.o)-len(outStr)) + outStr
        print ("%s %s" % (inStr, outStr))
      print (".e\n")
      sys.stdout = original_stdout
    # tFile.close()
    if (col != "Instruction"): ## Skip espresso optimization for ascii Instruction
      eFileName = xls.yml[col]['ModuleName'] + ".esp"
      print("\tRunning Espresso optimization ..." + eFileName)

      with open(tmpPath+eFileName,"w") as f:
        f.flush()
        subprocess.run([esp,tmpPath+tFileName],stdout=f)
        f.close()
      cmd = esp + " " + tFileName + " > " + eFileName
      print("\t",cmd)
      #  os.system(cmd)

  def checkThread(self, genStr, casezList, thrNum):
    print("Class: Pla", "__name__ =", repr(__name__), "method = checkThread") if debug else 0
    log = open("thr%s.log" % thrNum, 'w')
    for v in casezList:
      regexStr = re.sub(r'_', r'', v)
      regexStr = re.sub(r'x', r'.', regexStr) + r' [01-]+'
      #print >>log, regexStr
      result = re.findall(regexStr, genStr)
      if len(result) > 1:
        print >>log, 'Error:'
        print >>log, v
        print >>log, '\n'.join(result)
    log.close()
    return

  def checkGenPla(self, genFile):
    genStr = open(genFile).read()
    print("Class: Pla", "__name__ =", repr(__name__), "method = checkGenPla") if debug else 0
    #print >>sys.stderr, genStr
    #casezList = self.inPla.keys()
    for i in range(0, 4):
      caseStart = (len(casezList)/4+1) * i
      caseEnd   = (len(casezList)/4+1) * (i+1) - 1
      subList = casezList[caseStart: caseEnd] if i<3 else casezList[caseStart:]
      t = threading.Thread(target=self.checkThread, args=(genStr,subList,i))
      t.start()

  # Convert PLA to verilog boolean expression
  # Save return a hash, output port is key, expression is value
  def pla2vCase(self,xls,col):
    print("Class: Pla", "__name__ =", repr(__name__), "method = pla2vCase") if debug else 0
    self.genPla(xls,col)
    if (col == "Instruction"): ## Directly read .tmp file for ascii Instructions
      eFileName = tmpPath+xls.yml[col]['ModuleName'] + ".tmp"
    else:
      eFileName = tmpPath+xls.yml[col]['ModuleName'] + ".esp"
    #print("Debug> Reading .esp file %s" % (eFileName))
    eFile = open(eFileName, 'r')

    # Read PLA, translate it to verilog style
    exprHash = {}
    OutLine=[]
    line = eFile.readline()
    while line:
      if (col == "Instruction"):
        result = re.search(r'(^[01-]+) ([\S]+)', line)
        if result:
          inputValue = list(result.group(1))
          self.outPla[result.group(1)] = result.group(2)
          for q in range(len(inputValue)):
            if (inputValue[q] != '-'):
              self.casez[0][self.i[q]] = q
      else:
        result = re.search(r'([01-]+) ([01-]+)', line)
        if result:
          inputValue = list(result.group(1))
          outputValue = list(result.group(2))
          self.outPla[result.group(1)] = result.group(2)
          tmpList={}
          for p in range(len(outputValue)):
            if outputValue[p] == '1':
              for q in range(len(inputValue)):
                if (inputValue[q] != '-'):
                  #tmpList.append(self.i[q])
                  self.casez[p][self.i[q]] = q
              # Check and tie output to 1'b1 if all input is DC
              # if len(tmpList) == 0:
              #     tmpList.append("1'b1")
              # try:
              #     exprHash[self.o[p]].append(' & '.join(tmpList))
              # except:
              #     exprHash.update({self.o[p]:[]})
              #     exprHash[self.o[p]].append(' & '.join(tmpList))
      line = eFile.readline()
    eFile.close()

    # vFileName = col + ".esp.v"
    if xls.vFile:
      sys.stdout = xls.vFile
      print()
      print("module autogen_" + xls.yml[col]['ModuleName'] + " (")
      for x in self.inList:
        result = re.search(r'(\w+)(\[\d+:\d+\])', x)
        print ('input '+result.group(2)+' '+result.group(1)+',')
      for x in self.outList.keys():
        #print ("output reg [%s] %s') % (str(self.outList[x]), str(x))
        print ('output reg ['+self.outList[x]+"] "+x)
      print(");")
      print(spyInstrDisable) if col=="Instruction" else 0
      print("\nalways_comb begin")

      for x in range(len(self.o)):
        terms=0
        width=1
        cStr='casez({'
        for k,y in sorted(self.casez[x].items(),key=lambda i: i[1]):
          cStr += k+', '
          terms += 1
        if not terms:
          kStr = self.o[x] + " = " + str(width) + "\'b0; // Assigning Default value of 0"
          print(kStr)
          print('// No logic driving this. Skipping Output for '+self.o[x])
          print('')
        else:
          ## Input case Line
          print("\t"+cStr[:-2]+"})")
          ## Individual case statements
          for k in self.outPla.keys():
            kStr='    ' + str(terms) + "\'b"
            if self.outPla[k][x] == "0":
              continue
            for y in range(len(self.i)):
              kStr += k[y] if self.i[y] in self.casez[x].keys() else ''
            kStr += "  :  "
            if (col == "Instruction"):
              kStr += "Instruction = " + "\"" + self.outPla[k] + "\";"
            else:
              kStr += self.o[x] + " = " + str(width) + "\'b" + self.outPla[k][x] + ";"
            kStr = re.sub(r'-', r'?', kStr)
            print("\t"+kStr)
          ## Default statement
          kStr='    default : '
          if (col == "Instruction"):
            kStr += "Instruction = \"ILLEG\";"
          else:
            kStr += self.o[x] + " = " + str(width) + "\'b0;"
          print("\t"+kStr)
          print("\tendcase")
      print("end")
      print(spyInstrEnable) if col=="Instruction" else 0
      print("endmodule")
      sys.stdout = original_stdout
            
    # Check if any output port undriven
    for k in self.o:
        try:
            exprHash[k]
        except:
            exprHash.update({k: ["1'b0"]})

    for k in exprHash.keys():
        exprHash.update({k: "\n | ".join(exprHash[k])})

    return exprHash

## XLS object for storing the instruction spreadsheet
class seq():
  # Construct routine
  def __init__(self, xls):
    print("Class: seq", "__name__ =", repr(__name__), "method = __init__") if debug else 0

    self.vFile = xls.vFile
    ## Read in XLS file as Instruction DataFrame
    xlsheet = pd.read_excel(xls.xlsFile, sheet_name=None,dtype=str, engine='openpyxl')
    self.df = pd.concat([xlsheet['Seq']], axis=0, ignore_index=True)
    start=0
    table={}
    seqaddr={}
    table_name=''
    tables_col = self.df.iloc[:,0].to_list()
    seqaddr_col = self.df.iloc[:,1].to_list()
    # print (tables_col)
    # print (seqaddr_col)
    for i in range(len(tables_col)):
      if is_nan(tables_col[i]):
        continue
      if (table_name != ''):
        region=[start,i-2]
        df_table = self.df.iloc[region[0]+2:region[1]+2,:].set_axis(list(self.df.iloc[region[0]+1]), axis='columns')
        df_table.dropna(how='all',inplace=True) ## Delete all empty lines
        df_table.drop(df_table['SEQ'].str.startswith('#').where(lambda s: s).dropna().index, inplace = True) ## Delete all comment lines
        table[table_name] = df_table
        seqaddr[table_name] = seq_addr
      table_name=tables_col[i]
      seq_addr=seqaddr_col[i]
      start = i
    self.df = table
    self.addr = seqaddr

  # Deconstruct routine
  def __del__(self):
    print("Class: xls", "__name__ =", repr(__name__), "method = __del__") if debug else 0
    # for f in self.df:
    #   os.unlink(f)

  def gen_seq(self):
    print("Generating RTL sequencer modules")
    sys.stdout = self.vFile
    print("\nmodule autogen_sequence_uop (")
    print('input [7:0][6:3] SeqAddr,')
    print('output reg [7:0][35:0] uOp')
    print(');')
    seq=[]
    for x in range(8):
      seq.append([{-1:-1}])
    # seq = [[for x in range(8)] for y in range(128)] 
    for instr in self.df.keys():
      ## Prepare output Whisper Seq files to write alongside capturing the ROM Seq
      seqFileName = dvWhisperPath+ instr.lower()+'-uops'
      seqFile = open(seqFileName, 'w')
      for index, row in self.df[instr].iterrows():
        rom_addr = int(self.addr[instr], 2) + int(row['SEQ']) - 1
        rom_set = rom_addr//8
        rom_bank = rom_addr%8
        print("\t0x%s," %(row['Instr[35:0]']), file=seqFile)
        seq_line = ("\t  4'b%s : uOp[%d] =  36'h%s; // %s - %s - %s" % (format(rom_addr, '07b')[:4], rom_bank, row['Instr[35:0]'], instr, row['SEQ'], row['Description']))
        # print("%d : seq[%d][%d] = %s" %(rom_addr, rom_bank, rom_set, seq_line))
        seq[rom_bank].append({rom_set: seq_line})
      seqFile.close()

    for bank in range(8):
      print("\nalways_comb begin : Seq%d" %(bank))
      print("\tcasez({SeqAddr[%d]})" %(bank))
      for bank_hash in seq[bank]:
        for line in bank_hash.keys():
          if (line >= 0):
            print(bank_hash[line])
      print("\t  default : uOp[%d] = 'x;" %(bank))
      print("\tendcase")
      print("end // always")
    print("endmodule")
    sys.stdout = original_stdout

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Auto Generate Decode RTL modules from XLS.')
  parser.add_argument("-yml", "--yml", type=str, help="Decode YML file")
  parser.add_argument("-cfg", "--cfg", type=str, help="[OPTIONAL: Core Version. eg core_v2")
  parser.add_argument("-nodnd", "--nodnd", action="store_true", default=False, help="[OPT] Disable Auto download of xls files from Sharepoint")
  args = parser.parse_args()

  if not args.yml:
    print("Please provide YML file as an argument")
    exit()
  

  newYml = yml(args)
  newXls = xls(newYml)
  newXls.readDef()
  newXls.writeVerilogHeader()
  cols = newYml.ctrl['Cols']
  for col in cols.keys():
    print("Generating RTL for ",[col])
    newPla = pla(cols[col])
    print("After calling pla")
    val = newPla.pla2vCase(newXls,col)
  print("Generating RTL Sequencer modules ")
  newSeq = seq(newXls)
  newSeq.gen_seq()
  newXls.writeVerilogFooter()
  print ("OUTPUT FILES:")
  print ("\t%s    - Autogen RTL" % newXls.vFileName)
  print ("\t%s    - Log file for possible collisions. Fix in yml constraints" % newXls.lFileName)
  del newXls
