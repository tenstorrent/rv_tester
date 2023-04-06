load("@gen-csr//:csr_defs.bzl", "create_reg_uvm", "create_reg_env", "create_reg_rtl", "create_reg_constraint", "create_reg_hit", "create_reg_sv_header", "create_reg_c_header", "create_reg_html")

def gen_csr_modules(
    name,
    src,
    template = "@gen-csr//templates:reg_constraints",
    modules  = [ 
           "de",
           "ls",
           "fe",
           "ex"
            ],
):

    rule_list = []

    for module in modules:
        src_file = src

        rule_name = module+"_uvm" 
        out_file  = module+'_csr_ral.sv'

        create_reg_uvm(
            name   = rule_name,
            src    = src_file,
            out    = out_file,
            module = module,
        )
        rule_list.append(rule_name)

        rule_name = module+"_constraint" 
        out_constraints_file  = module+'_csr_constraints.sv'
        out_c_header_file  = module+'_csr.h'
        create_reg_constraint(
            name   = rule_name,
            src    = src_file,
            out_constraints = out_constraints_file,
            out_c_header = out_c_header_file,
            module = module,
        )
        rule_list.append(rule_name)

        rule_name = module+"_env" 
        out_pkg_file  = module+'_csr_pkg.sv'
        out_env_file  = module+'_csr_env.sv'
        out_build_file  = module+'_BUILD.bazel'
        create_reg_env(
            name   = rule_name,
            out_pkg = out_pkg_file,
            out_env = out_env_file,
            out_build = out_build_file,
            module = module,
        )
        rule_list.append(rule_name)


        rule_name = module+"_hit" 
        out_file_hit  = module+'_csr_hit.sv'
        out_file_serial  = module+'_csr_serial.sv'
        create_reg_hit(
            name   = rule_name,
            src    = src_file,
            out_hit = out_file_hit,
            out_serial = out_file_serial,
            module = module,
        )
        rule_list.append(rule_name)

        rule_name = module+"_rtl" 
        out_file  = module+'_csr.sv'
        create_reg_rtl(
            name   = rule_name,
            src    = src_file,
            out    = out_file,
            module = module,
        )
        rule_list.append(rule_name)

        rule_name = module+"_sv_header" 
        out_file  = module+'_csr_pkg.svh'
        create_reg_sv_header(
            name   = rule_name,
            src    = src_file,
            out    = out_file,
            module = module,
        )
        rule_list.append(rule_name)

        #rule_name = module+"_c_header" 
        #out_file  = module+'_csr.h'
        #create_reg_c_header(
        #    name   = rule_name,
        #    src    = src_file,
        #    out    = out_file,
        #    module = module,
        #)
        #rule_list.append(rule_name)

        rule_name = module+"_html" 
        out_file  = module+'_csr.html'
        create_reg_html(
            name   = rule_name,
            src    = src_file,
            out    = out_file,
            module = module,
        )
        rule_list.append(rule_name)
    return rule_list
