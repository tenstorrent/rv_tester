`ifndef DMI_TB_WRITES_UNSUPPORTED
module dmi_driver (
    input logic                     clk,
    input logic                     reset_n,

    input logic [31:0]              rand_dmi_driver_dly,
    input logic [31:0]              hart_enable_mask,
    
    input logic                     dmi_req_ready,
    input logic                     dmi_resp_valid,
    input rv_tester_pkg::dmi_resp_t dmi_resp,

    output logic                          dmi_req_valid,
    output rv_tester_pkg::dmi_req_t       dmi_req,
    output logic                          dmi_resp_ready,
    output logic                    [7:0] misc_signals,

    output logic                    dmi_status,
    output logic [31:0]             dmi_commands_in_queue,

    input rv_tester_pkg::dm_write_t trickbox_dmi_write
);

  // -----------------------------
  // DMI Stimulus
  // -----------------------------
  rv_tester_pkg::dmi_req_t  command;
  rv_tester_pkg::dmi_resp_t response;

  rv_tester_pkg::dmi_req_t  command_queue [$];
  rv_tester_pkg::dmi_resp_t response_queue[$];

  rv_tester_pkg::dmi_req_t single_step_ahead_command_queue[$], single_step_quit_command_queue[$];
  rv_tester_pkg::dmi_req_t
      single_step_ahead_command_queue_backup[$], single_step_quit_command_queue_backup[$];

  logic command_trigger, response_trigger;
  logic [31:0] clk_cnt = '0;
  logic halt_req, resume_req, abstr_cmd_req, poll, poll_p2, ndm_reset_init, ndm_reset_ack, ndm_reset_assert_done, ndm_reset_priority, poll_reset_completion, ndmreset_halt_req;
  logic [31:0] ext_trig_delay;
  logic [31:0] single_step_instr_cnt, single_step_executed_cnt;
  logic [31:0] ahead_queue_cnt, quit_queue_cnt, cnt;
  logic single_step_started, single_step_quit;
  logic abs_read, abs_write, abs_read_data, sdtrig_fire, halted_sdtrig;
  logic cores_in_halt_group, core_haltg_hreq, cores_in_resume_grp, core_resumeg_rreq, ack_havereset, remove_core_from_haltg, remove_core_from_resumeg;
  logic [7:0] core_in_halt_group, core_in_resume_grp, core_halted, core_resumed, core_ignore_hreq, core_ignore_rreq;
  logic [9:0] core_hg_check, core_rg_check;
  logic [2:0] core_halt_index, core_resume_index;
  logic tdata1_write, check_trigger_type, mcontrol6_trigger, trigger_to_fire, trigger_fired_halted, check_cause_trigger, cause_trigger, to_check_cause;
  logic dcsr_abscmd, dcsr_write, ss_step_bit, core_to_halt_after_ss, core_halted_after_ss;
  logic check_step_core, core_hg_resumed_ss, core_haltsum_ss, core_check_haltsum_ss;
  logic [2:0] core_ss_index, core_halted_ss, core_halted_sdtrig;
  logic tselect_core, tselect_core_complete, core_rg_halt_sdtrig, core_haltsum_sdtrig, check_haltsum_sdtrig;
  logic check_hartsellen, check_dmstatus_disc, hart_discovery, dmcontrol_hartsel;

  int file_descr, count_hart_enable_mask;

  typedef struct packed {
    logic [15:0] reg_addr;
    logic [63:0] reg_data;
  } abs_reg_out;

  abs_reg_out abs_data_temp_packet, abs_reg_out_queue[$];

  initial begin
    dmi_req_valid <= 'h0;
    dmi_resp_ready <= 'h0;
    command_trigger <= 'h0;
    clk_cnt <= 'h0;
    halt_req <= 0;
    resume_req <= 0;
    abstr_cmd_req <= 0;
    poll <= 0;
    poll_p2 <= 0;
    poll_reset_completion <= 0;
    ndm_reset_init <= 0;
    ndm_reset_ack <= 0;
    ext_trig_delay <= 0;
    single_step_executed_cnt <= 0;
    single_step_started <= 0;
    single_step_quit <= 0;
    abs_read <= 0;
    abs_write <= 0;
    abs_read_data <= 0;
    ndm_reset_priority <= 0;
    ndmreset_halt_req <= 0;
    cores_in_halt_group <= 0;
    core_haltg_hreq <= 0;
    cores_in_resume_grp <= 0;
    core_resumeg_rreq <= 0;
    core_in_halt_group <= 0;
    core_in_resume_grp <= 0;
    core_halted <= 0;
    core_resumed <= 0;
    core_ignore_hreq <= 0;
    core_ignore_rreq <= 0;
    remove_core_from_haltg <= 0;
    remove_core_from_resumeg <= 0;
    ack_havereset <= 0;
    sdtrig_fire <= 0;
    halted_sdtrig <= 0;
    tdata1_write <= 0;
    check_trigger_type <= 0;
    mcontrol6_trigger <= 0;
    trigger_to_fire <= 0;
    trigger_fired_halted <= 0;
    check_cause_trigger <= 0;
    cause_trigger <=0;
    to_check_cause <= 0;
    dcsr_abscmd <= 0;
    dcsr_write <= 0;
    ss_step_bit <= 0;
    core_to_halt_after_ss <= 0;
    core_halted_after_ss <= 0;
    check_step_core <= 0;
    core_ss_index <= 0;
    core_hg_resumed_ss <= 0;
    core_haltsum_ss <= 0;
    core_check_haltsum_ss <= 0;
    core_halted_ss <= 0;
    tselect_core <= 0;
    tselect_core_complete <= 0;
    core_rg_halt_sdtrig <= 0;
    core_haltsum_sdtrig <= 0;
    check_haltsum_sdtrig <= 0;
    core_halted_sdtrig <= 0;
    check_hartsellen <= 0;
    check_dmstatus_disc <= 0;
    hart_discovery <= 0;
    dmcontrol_hartsel <= 0;
  end

  initial begin
    count_hart_enable_mask = $countones(hart_enable_mask);
  end

  assign misc_signals[0] = poll;
  assign dmi_status = poll;

  always @(posedge clk) begin
    dmi_commands_in_queue = command_queue.size();
  end

  always @(posedge trickbox_dmi_write.dm_wvalid) begin
    // $display("DMI Write Data = %h \t %h",trickbox_dmi_write.dm_wdata, trickbox_dmi_write.dm_wdata[63:62]);
    if (trickbox_dmi_write.dm_wdata[63:59] === 'b00000) begin
      $display("[DMI Driver] Push In Command Queue");
      command_queue.push_back(rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata));
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b10000) begin
      //trigger dmi execution
      $display("[DMI Driver] Trigger Command Execution");
      command_trigger = 1;
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b01000) begin
      $display("[DMI Driver] Delaying the Trigger to Debugger");
      ext_trig_delay = clk_cnt + trickbox_dmi_write.dm_wdata[31:0];
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00010) begin
      single_step_ahead_command_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the single step (ahead) queue, size=%h",
               single_step_ahead_command_queue.size());
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00100) begin
      single_step_quit_command_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the single step (quit) queue, size=%h",
               single_step_quit_command_queue.size());
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00001) begin
      $display("[DMI Driver] Single steping mode enabled, getting count value");
      single_step_instr_cnt = trickbox_dmi_write.dm_wdata[31:0];
    end
  end

  always @(negedge clk) begin
    if (reset_n)
      clk_cnt = 0;
    else
      clk_cnt = clk_cnt + 1;
    if (clk_cnt !== 0 && clk_cnt === ext_trig_delay) begin
      command_trigger = 1;
      $display("[DMI Driver] The delay was executed and asserting the trigger");
    end
  end

  task do_file_writes();
    begin
      file_descr = $fopen("./abs_req_out.txt", "w+");
      for (int i = 0; i < abs_reg_out_queue.size(); i = i + 1) begin
        $fdisplay(file_descr, "Addr:%0h Data:%0h", abs_reg_out_queue[i].reg_addr,
                  abs_reg_out_queue[i].reg_data);
      end
      $fclose(file_descr);
    end
  endtask : do_file_writes

  task drive_dmi_cmd(input rv_tester_pkg::dmi_req_t cmd);
    begin
      repeat (rand_dmi_driver_dly) begin
        @(posedge clk);
      end
      @(posedge clk) dmi_req_valid <= '1;
      dmi_req <= cmd;
      wait (dmi_req_ready == 1);
      @(posedge clk) dmi_req_valid <= '0;
      wait (dmi_resp_valid == 1);
      @(posedge clk) dmi_resp_ready <= 1;
      response_queue.push_back(dmi_resp);
      @(posedge clk) dmi_resp_ready <= 0;
    end
  endtask : drive_dmi_cmd

  task is_poll_needed(input rv_tester_pkg::dmi_req_t cmd);
    begin
      //decode request type
      if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[31] === '1 && cmd.data[1] === '1) begin
        $display("[Poll] Seen Halt Req and poll_p2, Doing Poll for halt req after ndmreset");
        ndmreset_halt_req = 1;
        poll = 1;
      end else if(cmd.addr === 'h10 && cmd.op === 'h2 && sdtrig_fire === 'h1) begin
        $display("[Poll] Check if the core is halted through sdtrig");
        sdtrig_fire = 0;
        halted_sdtrig = 1;
        poll = 1;
        $display("[Poll] Clearing sdtrig_fire = 0");
        $display("[Poll] Setting halted_sdtrig = 1");
      end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[31] === '1 && cmd.data[1] === '0) begin
        core_hg_check = cmd.data[25:16];
        if(~core_in_halt_group[core_hg_check]) begin
          $display("[Poll] Seen Halt Req, Doing Poll");
          halt_req = 1;
          poll = 1;
        end else begin
          $display("[Poll] Core in halt group gets a haltreq, Doing Poll");
          core_haltg_hreq = 1;
          poll = 1;
        end
      end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[30] === '1) begin
        core_rg_check = cmd.data[25:16];
        if(~core_in_resume_grp[core_rg_check]) begin
          $display("[Poll] Seen Resume Req, Doing Poll");
          resume_req = 1;
          poll = 1;
        end else begin
          $display("[Poll] Core in resume group gets a resumereq, Doing Poll");
          core_resumeg_rreq = 1;
          poll = 1;
        end
      end else if (cmd.addr === 'h17 && cmd.op === 'h2) begin
        $display("[Poll] Seen Abstract Command Req, Doing Poll");
        abstr_cmd_req = 1;
        poll = 1;
        if (cmd.data[31:24] === 'h0 && cmd.data[17] === 'h1) begin
          //Seen an abstract reg command with rd/write
          $display("[Poll] Seen an abstract command with rd/write");
          if (cmd.data[16] === 'h1) begin
            $display("[Poll] Seen the abstract command with write");
            abs_write = 1;
            if(cmd.data[15:0] === 'h07a1) begin
              $display("[sdtrig:Poll] Seen an abstract command write on tdata1");
              tdata1_write = 1;
            end else if (cmd.data[15:0] === 'h07b0) begin
              $display("[Poll] Seen the abstract command with write on dcsr");
              dcsr_abscmd = 1;
            end else if(cmd.data[15:0] === 'h07a0)begin
              tselect_core = 1;
              $display("[sdtrig:Poll] Check which core has sdtrig configurations");
            end
          end else if (cmd.data[16] === 'h0) begin
            $display("[Poll] Seen the abstract command with read");
            abs_read = 1;
            abs_data_temp_packet.reg_addr = cmd.data[15:0];
            // if(cmd.data[15:0] === 'h07a2) begin
            //   sdtrig_fire = 1;
            //   $display("[Poll] Setting sdtrig_fire = 1");
            // end
            if(to_check_cause && cmd.data[15:0] === 'h7b0) begin
              to_check_cause = 0;
              check_cause_trigger = 1;
            end
          end
        end
      end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[1] === 'h1 ) begin
        $display("[Poll] Making NdmReset = 1, Doing Poll");
        ndm_reset_init = 1;
        ndm_reset_assert_done = 1;
        poll = 1;
        if(ss_step_bit) begin
          ss_step_bit = 0;
          $display("[Poll] Step field gets cleared with ndmreset");
        end
      end else if (ndm_reset_assert_done === 'h1 && cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[1] === 'h0 ) begin
        $display("[Poll] Making NdmReset = 0, Doing Poll");
        ndm_reset_assert_done = 0;
        ndm_reset_ack = 1;
        poll = 1;
      end else if (cmd.addr === 'h32 && cmd.op === 'h2 && ~cmd.data[11] && cmd.data[2:1] === 'h3 ) begin
        $display("Core entering halt group");
        cores_in_halt_group = 1;
        poll = 1;
      end else if (cmd.addr === 'h32 && cmd.op === 'h2 && ~cmd.data[11] && cmd.data[2:1] === 'h1) begin
        $display("Removing core from haltgroup");
        remove_core_from_haltg = 1;
        poll = 1;
      end else if(cmd.addr === 'h32 && cmd.op === 'h2 && cmd.data[11] && cmd.data[6:2] === 'h1 && cmd.data[1]) begin
        $display("Core entering resume group");
        cores_in_resume_grp = 1;
        poll = 1;
      end else if(cmd.addr === 'h32 && cmd.op === 'h2 && cmd.data[11] && cmd.data[6:2] === 'h0) begin
        $display("Removing core from resume group");
        remove_core_from_resumeg = 1;
        poll = 1;
      end else if(cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[28]) begin
        $display("[Poll] Acknowledge havereset");
        ack_havereset = 1;
        poll = 1;
      end else if(dcsr_abscmd && cmd.addr === 'h16 && cmd.op === 'h1) begin
        $display("[Single step] step bit configured in dcsr");
        dcsr_abscmd = 0;
        dcsr_write = 1;
        poll = 1;
      end else if(core_to_halt_after_ss && cmd.addr === 'h11 && cmd.op === 'h1 && ~trigger_to_fire) begin
        $display("[Single step] Core resuming after step configuration");
        poll = 1;
      end else if(cmd.addr === 'h16 && cmd.op === 'h1 && tdata1_write) begin
        $display("trigger type is configured in tdata1");
        tdata1_write = 0;
        check_trigger_type = 1;
        poll = 1;
      end else if(cmd.addr === 'h16 && cmd.op === 'h1 && tselect_core) begin
        $display("trigger type is configured in tdata1");
        tselect_core = 0;
        tselect_core_complete = 1;
        poll = 1;
      end else if(trigger_to_fire && cmd.addr === 'h11 && cmd.op === 'h1) begin
        $display("[Sdtrig] Core resuming after sdtrig configuration");
        poll = 1;
        trigger_fired_halted = 1;
        trigger_to_fire = 0;
        if(core_to_halt_after_ss) begin
          core_to_halt_after_ss = 0;
          $display("[Sdtrig] Sdtrig takes priority over single stepping");
        end
      end else if(check_cause_trigger && cmd.addr === 'h4 && cmd.op === 'h1) begin
        poll = 1;
        check_cause_trigger = 0;
        cause_trigger = 1;
      end else if(ss_step_bit && cmd.addr === 'h10 && cmd.op === 'h1) begin
        poll = 1;
        check_step_core = 1;
        ss_step_bit = 0;
        $display("[ss_hg] check_step_core set, check which core to single step");
      end else if(core_haltsum_ss && cmd.addr === 'h11 && cmd.op === 'h1) begin
        poll = 1;
        core_haltsum_ss = 0;
        core_check_haltsum_ss = 1;
        $display("[ss_hg] core_check_haltsum_ss is set");
      end else if(core_haltsum_sdtrig && cmd.addr === 'h11 && cmd.op === 'h1) begin
        poll = 1;
        check_haltsum_sdtrig = 1;
        core_haltsum_sdtrig = 0;
      end else if(cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[23:16] === 'hff) begin
        poll = 1;
        check_hartsellen = 1;
        $display("check_hartsellen is set");
      end else if(check_dmstatus_disc && cmd.addr === 'h10 && cmd.op === 'h2) begin
        poll = 1;
        hart_discovery = 1;
        dmcontrol_hartsel = cmd.data[18:16];
        $display("hart_discovery is set, dmcontrol_hartsel:%h ", dmcontrol_hartsel);
      end
    end
  endtask : is_poll_needed

  task do_polling();
    begin
      $display("[Poll] Starting poll for halt:%h resume:%h abstract:%h", halt_req, resume_req,
               abstr_cmd_req);
      while (poll) begin
        //READ DMSTATUS
        @(posedge clk) dmi_req_valid <= '1;
        if ((halt_req | resume_req) && ~cores_in_halt_group && ~cores_in_resume_grp) begin
          //  $display("dmstatus haltreq %d resumereq %d\n",halt_req,resume_req);
          dmi_req <= 41'h4500000000;
        end else if (abstr_cmd_req) begin
          // $display("abstractcs busy %d\n",abstr_cmd_req);
          dmi_req <= 41'h5900000000;
        end else if (abs_read_data) begin
          $display("[Poll] Doing abstract read data for data0");
          dmi_req <= 41'h1100000000;
        end else if (ndm_reset_init) begin
          $display("[Poll] Read dmstatus to check for ndmresetpending to be 1");
          dmi_req <= 41'h4500000000;
        end else if (ndm_reset_ack) begin
          $display("[Poll] Read dmstatus to check for ndmresetpending to be 0");
          dmi_req <= 41'h4500000000;
        end else if (ndmreset_halt_req) begin
          $display("[Poll] Read dmstatus to check for ndmresetpending to be 1 and halted state");
          dmi_req <= 41'h4500000000;
        end else if (cores_in_halt_group) begin
          $display("[Poll] Read dmcontrol to see which cores are added to halt group");
          dmi_req <= 41'h4100000000; //TODO(Bavani): Revisit
        end else if (remove_core_from_haltg) begin
          $display("[Poll] Read dmcontrol to see which cores are removed from halt group");
          dmi_req <= 41'h4100000000;
        end else if (core_haltg_hreq) begin
          $display("[Poll] Read Haltsum reg");
          dmi_req <= 41'h10100000000;
        end else if (cores_in_resume_grp) begin
          $display("[Poll] Read dmcontrol to see which cores are added to resume group");
          dmi_req <= 41'h4100000000; //TODO(Bavani): Revisit
        end else if (remove_core_from_resumeg) begin
          $display("[Poll] Read dmcontrol to see which cores are removed from resume group");
          dmi_req <= 41'h4100000000;
        end else if (core_resumeg_rreq) begin
          $display("[Poll] Read Haltsum reg");
          dmi_req <= 41'h10100000000;
        end else if (ack_havereset) begin
          $display("[Poll] Read dmstatus to clear anyhavereset and allhavereset");
          dmi_req <= 41'h4500000000;
        end else if (halted_sdtrig) begin
          $display("[Poll] dmstatus to check if the core is getting halted through sdtrig");
          dmi_req <= 41'h4500000000;
        end else if (dcsr_write) begin
          $display("[Poll] data0 to check if the step bit is set");
          dmi_req <= 41'h1100000000;
        end else if (core_to_halt_after_ss) begin
          $display("[Poll] check for the core to be halted after ss");
          dmi_req <= 41'h4500000000;
        end else if (check_trigger_type) begin
          $display("[Poll] data1 to check if the trigger type is mcontrol6");
          dmi_req <= 41'h1500000000;
        end else if (trigger_fired_halted) begin
          $display("[Poll] dmstatus to check if the core is getting halted through sdtrig");
          dmi_req <= 41'h4500000000;
        end else if(cause_trigger) begin
          $display("[Poll] dcsr to check if cause is 2(trigger)");
          dmi_req <= 41'h1100000000;
        end else if(check_step_core) begin
          dmi_req <= 41'h4100000000;
          $display("[Poll] check which core to single step in hartgroup");
        end else if(core_check_haltsum_ss) begin
          $display("[Poll] Read Haltsum reg for single step");
          dmi_req <= 41'h10100000000;
        end else if(tselect_core_complete) begin
          $display("[Poll] check heartsel in dmcontrol for sdtrig config");
          dmi_req <= 41'h4100000000;
        end else if(check_haltsum_sdtrig) begin
          $display("[Poll] Read Haltsum reg for sdtrig");
          dmi_req <= 41'h10100000000;
        end else if(check_hartsellen) begin
          $display("[Poll] Check dmcontrol hartsellen based on DM fuse enable");
          dmi_req <= 41'h4100000000;
        end else if(hart_discovery) begin
          $display("[Poll] Check dmstatus to see if the enabled hart is running");
          dmi_req <= 41'h4500000000;
        end
        wait (dmi_req_ready == 1);
        @(posedge clk) dmi_req_valid <= '0;
        wait (dmi_resp_valid == 1);
        @(posedge clk) dmi_resp_ready <= 1;
        @(posedge clk) dmi_resp_ready <= 0;
        //check dmstatus
        // $display("\ndmi resp %h\n",dmi_resp.data);
        if (resume_req && dmi_resp.data[17:16] === 2'b11) begin
          resume_req = 0;
          poll = 0;
          if(mcontrol6_trigger) begin
            trigger_to_fire = 1;
            mcontrol6_trigger = 0;
            $display("[Poll] trigger_to_fire is set");
          end
          $display("[Poll] Clear Resume Req Poll");
          do_file_writes();
          if(ss_step_bit) begin
            core_to_halt_after_ss = 1;
            $display("core_to_halt_after_ss is set");
          end
          if(core_halted_after_ss) begin
            core_halted_after_ss = 0;
            $display("core_halted_after_ss is cleared");
          end
        end else if (halt_req && dmi_resp.data[9:8] === 2'b11) begin
          halt_req = 0;
          poll = 0;
          $display("[Poll] Clear Halt Req Poll");
        end else if (abstr_cmd_req && dmi_resp.data[12] === 1'b0) begin
          abstr_cmd_req = 0;
          $display("[Poll] Clear Abstract Command Req Poll");
          $display("[Poll] Reading the data output of the abstract command");
          if (abs_read === 'h1) begin
            abs_read_data = 1;
          end else begin
            poll = 0;
          end
        end else if (abs_read_data) begin
          abs_data_temp_packet.reg_data = dmi_resp.data;
          abs_reg_out_queue.push_back(abs_data_temp_packet);
          abs_read_data = 0;
          poll = 0;
        end else if (ndm_reset_init && dmi_resp.data[24] === 1'b1) begin
          ndm_reset_init = 0;
          //Holding ndmreset = 1 for 100 cycles
          for(int ii=0;ii<100;ii++)
          begin
            $display("[Poll] Executing for loop in driver");
            @(posedge clk) begin
              if (ii === 99)
              begin
                poll = 0;
                poll_p2 = 1;
                $display("[Poll] Cleared poll as ndmresetpending is 1 and started");
              end
            end
          end
        end  else if (ndm_reset_ack && dmi_resp.data[24] === 1'b0) begin
          ndm_reset_ack = 0;
          poll = 0;
          $display("[Poll] Cleared poll as ndmresetpending is 0 and ended");
        end else if (ndmreset_halt_req && dmi_resp.data[24] === 1'b1) begin
          ndmreset_halt_req = 0;
          poll = 0;
          $display("[Poll] Cleared poll as ndmresetpending is 1 ndmreset initiated and core is in halted state");
        end else if(cores_in_halt_group) begin
          core_halt_index = dmi_resp.data[18:16];
          core_in_halt_group[core_halt_index] = 1;
          if(core_ignore_hreq[core_halt_index] == 1) begin
            core_ignore_hreq[core_halt_index] = 0;
          end
          poll = 0;
          cores_in_halt_group = 0;
          $display("[Poll] Adding core%0d to halt group", core_halt_index);
        end else if(core_haltg_hreq) begin
          for(int ii=0; ii<8; ii++)begin
            if(core_in_halt_group[ii] && dmi_resp.data[ii])begin
              core_halted[ii] = 1;
              $display("[Poll] core%0d in halt group is halted", ii);
            end else if(!core_in_halt_group[ii]) begin
              core_ignore_hreq[ii] = 1;
              $display("[Poll] core%0d is ignored as it's not part of halt group", ii);
            end
          end
          if((core_halted[0] || core_ignore_hreq[0]) && (core_halted[1] || core_ignore_hreq[1]) &&
          (core_halted[2] || core_ignore_hreq[2]) && (core_halted[3] || core_ignore_hreq[3]) && (core_halted[4] || core_ignore_hreq[4]) &&
          (core_halted[5] || core_ignore_hreq[5]) && (core_halted[6] || core_ignore_hreq[6]) && (core_halted[7] || core_ignore_hreq[7])) begin
            poll = 0;
            core_haltg_hreq = 0;
            core_halted = 0;
            core_ignore_hreq = 0;
            $display("[Poll] All cores in halt group are halted");
          end
        end else if (remove_core_from_haltg) begin
          core_halt_index = dmi_resp.data[18:16];
          core_in_halt_group[core_halt_index] = 0;
          remove_core_from_haltg = 0;
          poll = 0;
        end else if(cores_in_resume_grp) begin
          core_resume_index = dmi_resp.data[18:16];
          core_in_resume_grp[core_resume_index] = 1;
          if(core_ignore_rreq[core_resume_index] == 1) begin
            core_ignore_rreq[core_resume_index] = 0;
          end
          poll = 0;
          cores_in_resume_grp = 0;
          $display("[Poll] Adding core%0d to resume group", core_resume_index);
        end else if(core_resumeg_rreq) begin
          for(int ii=0; ii<8; ii++) begin
            if(core_in_resume_grp[ii] && ~dmi_resp.data[ii])begin
              core_resumed[ii] = 1;
              $display("[Poll] core%0d in resume group is resumed", ii);
            end else if(!core_in_resume_grp[ii]) begin
              core_ignore_rreq[ii] = 1;
              $display("[Poll] core%0d is ignored as it's not part of resume group", ii);
            end
          end
          if((core_resumed[0] || core_ignore_rreq[0]) && (core_resumed[1] || core_ignore_rreq[1]) &&
          (core_resumed[2] || core_ignore_rreq[2]) && (core_resumed[3] || core_ignore_rreq[3]) && (core_resumed[4] || core_ignore_rreq[4]) &&
          (core_resumed[5] || core_ignore_rreq[5]) && (core_resumed[6] || core_ignore_rreq[6]) && (core_resumed[7] || core_ignore_rreq[7])) begin
            poll = 0;
            core_resumeg_rreq = 0;
            core_resumed = 0;
            core_ignore_rreq = 0;
            $display("[Poll] All cores in resume group are resumed");
          end
          if(core_hg_resumed_ss == 1) begin
            core_haltsum_ss = 1;
            core_hg_resumed_ss = 0;
            $display("[Poll] core_haltsum_ss is set");
          end
          if(mcontrol6_trigger && core_rg_halt_sdtrig) begin
            core_haltsum_sdtrig = 1;
            core_rg_halt_sdtrig = 0;
            mcontrol6_trigger = 0;
            $display("[Poll] core_haltsum_sdtrig is set ");
          end
        end else if (remove_core_from_resumeg) begin
          core_resume_index = dmi_resp.data[18:16];
          core_in_resume_grp[core_resume_index] = 0;
          remove_core_from_resumeg = 0;
          poll = 0;
        end else if(ack_havereset && ~dmi_resp.data[19:18])begin
          ack_havereset = 0;
          poll = 0;
          $display("[Poll] Clear Ack havereset Poll");
        // end else if(dmstatus_delay)begin
        //   for(int ii=0;ii<100;ii++)
        //   begin
        //     $display("[Poll] Executing for loop in driver for dmstatus");
        //     @(posedge clk) begin
        //       if (ii === 99)
        //       begin
        //         dmstatus_delay = 0;
        //         poll = 0;
        //         $display("[Poll] Adding 100 cycles delay");
        //       end
        //     end
        //   end
        // end else if(check_trigger_type && dmi_resp.data[31:28] === 'h6) begin
        //   mcontrol6_trigger = 1;
        //   check_trigger_type = 0;
        //   poll = 0;
        end else if(halted_sdtrig && dmi_resp.data[9:8] === 'h3)begin
          halted_sdtrig = 0;
          poll = 0;
          $display("[Poll] Clearing halted_sdtrig = 0");
        end else if(dcsr_write) begin
          if(dmi_resp.data[2] === 'h1) begin
            ss_step_bit = 1;
            $display("[Poll] step bit is set in dcsr");
          end else if(dmi_resp.data[2] === 'h0 && ss_step_bit) begin
            ss_step_bit = 0;
            $display("[Poll] step bit is cleared in dcsr");
          end
          poll = 0;
          dcsr_write = 0;
          $display("[Poll] dcsr_write polling completed");
        end else if(core_to_halt_after_ss && dmi_resp.data[9:8] === 2'b11) begin
          core_to_halt_after_ss = 0;
          core_halted_after_ss = 1;
          poll = 0;
          $display("[Poll] core_halted_after_ss is set");
        end else if(check_trigger_type) begin
          if(dmi_resp.data[31:28] === 'h6) begin
            mcontrol6_trigger = 1;
          end
          check_trigger_type = 0;
          poll = 0;
        end else if(trigger_fired_halted && dmi_resp.data[9:8] === 'h3)begin
          trigger_fired_halted =0;
          to_check_cause = 1;
          poll = 0;
          $display("[Poll:Sdtrig] Trigger fired and core entered debug mode");
        end else if(cause_trigger && dmi_resp.data[8:6] === 'h2) begin
          cause_trigger = 0;
          poll = 0;
          $display("[Poll:Sdtrig] core halt cause is set to 2(trigger)");
        end else if(check_step_core) begin
          core_ss_index = dmi_resp.data[18:16];
          core_hg_resumed_ss = 1;
          check_step_core = 0;
          poll = 0;
          $display("[Poll:ss_hg] core_hg_resumed_ss is set");
        end else if(core_check_haltsum_ss) begin
          for(int ii=0; ii<8; ii++) begin
            if(ii == core_ss_index) begin
              core_halted_ss[ii] = 1;
            end else begin
              core_halted_ss[ii] = 0;
            end
          end
          if(core_halted_ss && dmi_resp.data[2:0]) begin
            poll = 0;
            core_check_haltsum_ss = 0;
            core_halted_ss = 0;
            $display("[Poll:ss_hg] haltsum_ss = %h as expected", core_halted_ss);
          end
        end else if(tselect_core_complete) begin
          core_ss_index = dmi_resp.data[18:16];
          core_rg_halt_sdtrig = 1;
          tselect_core_complete = 0;
          poll = 0;
          $display("[Poll:sdtrig_hg] core_rg_halt_sdtrig is set");
        end else if(check_haltsum_sdtrig) begin
          for(int ii=0; ii<8; ii++) begin
            if(ii == core_ss_index) begin
              core_halted_sdtrig[ii] = 1;
            end else begin
              core_halted_sdtrig[ii] = 0;
            end
          end
          if(core_halted_sdtrig && dmi_resp.data[2:0]) begin
            poll = 0;
            check_haltsum_sdtrig = 0;
            core_halted_sdtrig = 0;
            $display("[Poll:ss_hg] haltsum_ss = %h as expected", core_halted_sdtrig);
          end
        end else if(check_hartsellen) begin
          if(count_hart_enable_mask <= 2 && dmi_resp.data[16] === 1 && dmi_resp.data[25:17] === 0) begin
            check_hartsellen = 0;
            check_dmstatus_disc = 1;
            poll = 0;
            $display("Hartsellen should be 'b1");
          end else if(count_hart_enable_mask <= 4 && dmi_resp.data[17:16] === 3 && dmi_resp.data[25:18] === 0) begin
            check_hartsellen = 0;
            check_dmstatus_disc = 1;
            poll = 0;
            $display("Hartsellen should be 'b11");
          end else if(count_hart_enable_mask <= 8 && dmi_resp.data[18:16] === 7 && dmi_resp.data[25:19] === 0 ) begin
            check_hartsellen = 0;
            check_dmstatus_disc = 1;
            poll = 0;
            $display("Hartsellen should be 'b111");
          end
        end else if(hart_discovery) begin
          $display("Chcking Hartsel in dmcontrol, hart_enable_mask[dmcontrol_hartsel]:%h, dmcontrol_hartsel:%h, dmi_resp.data[11:10]:%h", hart_enable_mask[dmcontrol_hartsel], dmcontrol_hartsel, dmi_resp.data[11:10]);
          if(hart_enable_mask[dmcontrol_hartsel] === 1 && dmi_resp.data[11:10] === 3)  begin
            hart_discovery = 0;
            poll = 0;
            dmcontrol_hartsel = 0;
            $display("Selected hart is running as it's enabled");
          end else if(hart_enable_mask[dmcontrol_hartsel] === 0 && dmi_resp.data[13:12] === 3 && dmi_resp.data[11:10] === 0) begin
            hart_discovery = 0;
            poll = 0;
            dmcontrol_hartsel = 0;
            $display("Selected hart is unavailable as it's disabled");
          end
          if(dmcontrol_hartsel === 7) begin
            check_dmstatus_disc = 0;
            $display("Clear check_dmstatus_disc");
          end
        end
      end
      $display("[Poll] Cleared poll for halt:%h resume:%h abstract:%h", halt_req, resume_req,
               abstr_cmd_req);
    end
  endtask : do_polling

  always @(posedge command_trigger) begin
    $display("[DMI Execution] Starting the execution of Debug Commands");
    if (command_trigger > 0) begin
      while (command_queue.size() > 0 && single_step_started != 1) begin
        command = command_queue.pop_front();
        $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                 command.data);
        if (command.op == 'h3) begin
          $display("[DMI Execution] Encountered Debug Checkopoint, Switching Control to Assembly");
          break;
        end else begin
          drive_dmi_cmd(command);
          is_poll_needed(command);
          if (poll) begin
            do_polling();
          end
          $display("[DMI Execution] Executed the command, commands left out in the queue=%h",
                   command_queue.size());
        end
      end
      command_trigger = 0;
      $display("[DMI Execution] Clear the Execution Trigger\n");
      @(posedge clk);
    end
  end

endmodule
`endif
