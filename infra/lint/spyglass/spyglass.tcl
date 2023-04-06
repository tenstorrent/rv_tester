set_option language_mode mixed
set_option designread_enable_synthesis no
set_option designread_disable_flatten no
set_option enableSV yes
set_option enableSV09 yes
set_option active_methodology $SPYGLASS_HOME/GuideWare/latest/block/rtl_handoff
set_option report_incr_messages no
set_parameter handle_large_bus yes

# FIXME - is this the right way to handle IP cells? Should we be reading the macro .lib instead?
set_option stop {tsmc_tsmc7*}
# FIXME - handle this better
set_option define TSMC_TSMC7_240H_STDCELLS
set_option stop {tt_libcell*}

# Increase the threshold for memory size
set_option mthresh 33280

# Ignore rules (FIXME: These should be reviewed)
set_option ignorerules NoGenLabel-ML
# W443 - X has no meaning in synthesis (We intentionally put X's in our case statements)
set_option ignorerules W443
set_option ignorerules NoAssignX-ML
set_option ignorerules ReserveName
# FileName and Module Name doesn't have to be same
set_option ignorerules STARC05-1.1.1.1
set_option non_lrm_options allow_assert_final

#turn off ASSERT are not synthesizable
set_option ignorerules SYNTH_5064

#allow Sync reset on flops
set_option ignorerules STARC-2.3.4.3
