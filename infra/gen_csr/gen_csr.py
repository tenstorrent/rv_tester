#!/usr/bin/env python3

import os
import subprocess
import shutil
import argparse

ROOTDIR      = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + "/"
sp_getxls    = "/tools_risc/common/bin/sp_getxls"
XLS_src_file ="https://tenstorrent.sharepoint.com/:x:/r/sites/RiscV/Shared%20Documents/Projects/ASC/RTL/AS_CSRs.xlsx"
XLS_dst_file = ROOTDIR+"gen/AS_CSRs.xlsx"

def gen_type_dict(src_file_func, dest_file_func, dest_folder_func):
    return {
        "src_file_name" : src_file_func,
        "dest_file_name": dest_file_func,
        "dest_folder"   : dest_folder_func
    }

def gen_module_dict(uvm_path, constraint_path, env_path, rtl_path, sv_header_path, c_header_path, html_path):
    return {
        "uvm"          : ROOTDIR+uvm_path,
        "constraints"  : ROOTDIR+constraint_path,
        "env"          : ROOTDIR+env_path,
        "rtl"          : ROOTDIR+rtl_path,
        "sv_header"    : ROOTDIR+sv_header_path,
        "c_header"     : ROOTDIR+c_header_path,
        "html"         : ROOTDIR+html_path
    }

MODULE_FILE            = ROOTDIR+"infra/gen_csr/modules.bzl"
TMP_MODULE_FILE        = ROOTDIR+"infra/gen_csr/tmp_modules.bzl"
BAZEL_OUT              = ROOTDIR+"bazel-bin/infra/gen_csr"

#List of generated outputs to actually store in source folders
EXTRACT_OUTPUTS        = [
    "uvm", "constraints", "env", "pkg", "build", "csr_hit", "csr_serial", "rtl", "sv_header", "c_header", "html"
]

MODULE_DEST_PATH       = {
    #                                     uvm_path,            constraints,        env,                 rtl_path, sv_header,        c_header,           html_path
    "mc"               : gen_module_dict("dv/mc/env/csr_lib", "dv/mc/env/csr_lib", "dv/mc/env/csr_lib", "rtl/csr", "rtl/csr",  "dv/mc/env/csr_lib", "gen/html"),
    "ls"               : gen_module_dict("dv/ls/env/csr_lib", "dv/ls/env/csr_lib", "dv/ls/env/csr_lib", "rtl/csr", "rtl/csr",  "dv/ls/env/csr_lib", "gen/html"),
    "fe"               : gen_module_dict("dv/fe/env/csr_lib", "dv/fe/env/csr_lib", "dv/fe/env/csr_lib", "rtl/csr", "rtl/csr",  "dv/fe/env/csr_lib", "gen/html"),
    "ch"               : gen_module_dict("dv/ch/env/csr_lib", "dv/ch/env/csr_lib", "dv/ch/env/csr_lib", "rtl/csr", "rtl/csr",  "dv/ch/env/csr_lib", "gen/html")
}

#Dictionary of functions to get the file name and destination folder of each output file type
DECODE_KEY             = {
    #File type                         Src file name                      Destination file name              Destination folder
    "uvm"              : gen_type_dict(lambda a: a+"_csr_ral.sv",         lambda a: a+"_csr_ral.sv",         lambda a: MODULE_DEST_PATH[a]["uvm"]),
    "constraints"      : gen_type_dict(lambda a: a+"_csr_constraints.sv", lambda a: a+"_csr_main.sv",        lambda a: MODULE_DEST_PATH[a]["constraints"]),
    "env"              : gen_type_dict(lambda a: a+"_csr_env.sv",         lambda a: a+"_csr_env.sv",         lambda a: MODULE_DEST_PATH[a]["env"]),
    "pkg"              : gen_type_dict(lambda a: a+"_csr_pkg.sv",         lambda a: a+"_csr_ral_pkg.sv",     lambda a: MODULE_DEST_PATH[a]["env"]),
    "build"            : gen_type_dict(lambda a: a+"_BUILD.bazel",        lambda a: "BUILD.bazel",           lambda a: MODULE_DEST_PATH[a]["env"]),
    "csr_hit"          : gen_type_dict(lambda a: a+"_csr_hit.sv",         lambda a: a+"_csr_hit.sv",         lambda a: MODULE_DEST_PATH[a]["rtl"]),
    "csr_serial"       : gen_type_dict(lambda a: a+"_csr_serial.sv",      lambda a: a+"_csr_serial.sv",      lambda a: MODULE_DEST_PATH[a]["rtl"]),
    "rtl"              : gen_type_dict(lambda a: a+"_csr.sv",             lambda a: a+"_csr.sv",             lambda a: MODULE_DEST_PATH[a]["rtl"]),
    "sv_header"        : gen_type_dict(lambda a: a+"_csr_pkg.svh",        lambda a: a+"_csr_pkg.svh",        lambda a: MODULE_DEST_PATH[a]["sv_header"]),
    "c_header"         : gen_type_dict(lambda a: a+"_csr.h",              lambda a: a+"_csr.h",              lambda a: MODULE_DEST_PATH[a]["c_header"]),
    "html"             : gen_type_dict(lambda a: a+"_csr.html",           lambda a: a+"_csr.html",           lambda a: MODULE_DEST_PATH[a]["html"])
}


parser = argparse.ArgumentParser()
parser.add_argument('-repo', '--repo_path', help='path to local gen-csr repo. Note: It has to under risc_p_cores', default = "")
parser.add_argument('-unit', '--module_list', help="comma delimited module list input.eg: \"fe,mc,ls,ch\"", type=str, default = "fe,mc,ls,ch")
parser.add_argument("-nodnd", "--nodnd", action="store_true", default=False, help="[OPT] Disable auto download of xls files")
parser.add_argument("-dnd_only", "--dnd_only", action="store_true", default=False, help="[OPT] Just download the excel file from sharepoint. Do not generate CSR files.")
parser.add_argument("-keep_xls", "--keep_xls", action="store_true", default=False, help="[OPT] Do not delete the downloaded XLS file")

args = parser.parse_args()

gen_csr_repo_path      = args.repo_path
input_module_list      = my_list = [item.strip() for item in args.module_list.split(',')]

## Read in XLS file as Instruction DataFrame
if not args.nodnd:
    print ("Downloading XLS file into " + XLS_dst_file)
    try:
      subprocess.run([sp_getxls,XLS_src_file,XLS_dst_file])
    except:
      print("Unable to download Xls file")
      print("Please manually download AS_CSRs.xlsx file (into gen/) and rerun with -nodnd option")
      exit()

if args.dnd_only:
    exit()

#Generate a module.bzl file with a list of the modules to generate CSRs for
#This list will be used by Bazel 

module_info = f"""\
modules = [
"""
for mod in input_module_list[0:len(input_module_list)-1]:
    module_info += f"""\
    "{mod}",    
"""
module_info +=f"""\
    "{input_module_list[-1]}"
]    
"""

#Save a copy of the default Module file
os.rename(MODULE_FILE, TMP_MODULE_FILE)

f = open(MODULE_FILE, 'w')
f.write(module_info)
f.close()

bazel_cmd = "infra/bazel-wrap build //infra/gen_csr:gen_csr_all"
if gen_csr_repo_path != "":
    bazel_cmd = bazel_cmd + " --override_repository=gen-csr=$(pwd)/" + gen_csr_repo_path

print("Bazel Cmd: " + bazel_cmd)
os.system(bazel_cmd)

#Restore default Module file
os.rename(TMP_MODULE_FILE, MODULE_FILE)

#Move the generated files to their respective folders
for mod in input_module_list:
    for op in EXTRACT_OUTPUTS:
        src_file_name  = os.path.join(BAZEL_OUT, DECODE_KEY[op]["src_file_name"](mod))
        dest_file_name = os.path.join(DECODE_KEY[op]["dest_folder"](mod), DECODE_KEY[op]["dest_file_name"](mod))
        if os.path.exists(dest_file_name):
            os.remove(dest_file_name)
        #os.rename(src_file_name, dest_file_name)
        shutil.copy(src_file_name, dest_file_name)

if os.path.exists(XLS_dst_file) and not args.keep_xls:
    os.remove(XLS_dst_file)
