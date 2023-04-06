flavors = ["v2", "v4", "v8"]
flavored_units = ["core", "ci", "fe", "ls", "mc", "vec"]

unit_map = {
    'core'                            : '//rtl/core'                                           ,
    'core_tb'                         : '//dv/core'                                            ,
    'old_core_l2_tb'                  : '//dv/core'                                            ,
    'old_core_tb'                     : '//dv/core'                                            ,
    'ci'                              : '//rtl/ci'                                             ,
    'ch'                              : '//rtl/ch'                                             ,
    'fe'                              : '//rtl/fe'                                             ,
    'ls'                              : '//rtl/ls'                                             ,
    'mc'                              : '//rtl/mc'                                             ,
    'mc_mul_tb'                       : '//rtl/mc/mc_mul_tb'                                   ,
    'vec'                             : '//rtl/vec'                                            ,
    'mc_tb'                           : '//dv/mc'                                              ,
    'ls_tb'                           : '//dv/ls'                                              ,
    'ls_rtl_tb'                       : '//dv/ls/backup'                                       ,
    'vec_tb'                          : '//dv/vec'                                             ,
    'fe_tb'                           : '//dv/fe'                                              ,
    'sc'                              : '@sharedcache//rtl'                                    ,
    'ncore_fabric'                    : '@arteris_ncore_fabric//'                              ,
    'fe_tb_tsmc'                      : '//dv/fe'                                              ,
    'assertion_macros_tb'             : '//dv/tb/assertion_macros'                             ,
    'mem_manager_example_tb'          : '//dv/tb/mem_manager_example'                          ,
    'logger_tb'                       : '//dv/tb/logger/test'                                  ,
    'sysmem_tb'                       : '//dv/tb/sysmem/test'                                  ,
    'lsf'                             : '//lsf'                                                ,
    'regr_check_tb'                   : '//dv/tb/regr_check'                                   ,
    'info_pass_tb'                    : '//dv/tb/info_pass'                                    ,
    'fml_ls'                          : '//fml/ls'                                             ,
    'fml_srb'                         : '//fml/misc/srb_fake/fml'                                             ,
    'fml_l2'                          : '//fml/l2'                                             ,
    'fml_fe'                          : '//fml/fe'                                             ,
    'fml_core'                        : '//fml/core'                                             ,
    'all'                             : '//dv'                                                 ,
    '__default'                       : 'all'                                                  ,
    'srb_fake'                        : '//fml/misc/srb_fake/rtl'                              ,
}

for unit in flavored_units:
    for suffix in ["", "_tb"]:
        for flavor in flavors:
            oname = f"{unit}{suffix}"
            fname = f"{oname}_{flavor}"
            if oname in unit_map and fname not in unit_map:
                unit_map[fname] = unit_map[oname]

#stays
workspace_name = "risc-p-cores"
