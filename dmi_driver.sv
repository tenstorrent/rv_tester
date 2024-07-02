`ifndef DMI_TB_WRITES_UNSUPPORTED
module dmi_driver (
    input logic                     clk,
    input logic                     reset,
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

  // always @(posedge clk) begin
  //   if(dmi_req_valid && dmi_req_ready) begin
  //     dmi_monitor_dpi(dmi_req.data,dmi_req.op,dmi_req.addr); //DPI-Call
  //   end
  // end

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
  logic [31:0] clk_cnt;
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

  int file_descr;

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
    clk_cnt = clk_cnt + 1;
    if (clk_cnt == ext_trig_delay) begin
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
          end else if (cmd.data[16] === 'h0) begin
            $display("[Poll] Seen the abstract command with read");
            abs_read = 1;
            abs_data_temp_packet.reg_addr = cmd.data[15:0];
            // if(cmd.data[15:0] === 'h07a2) begin
            //   sdtrig_fire = 1;
            //   $display("[Poll] Setting sdtrig_fire = 1");
            // end
          end
        end
      end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[1] === 'h1 ) begin
        $display("[Poll] Making NdmReset = 1, Doing Poll");
        ndm_reset_init = 1;
        ndm_reset_assert_done = 1;
        poll = 1;
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
        end
        wait (dmi_req_ready == 1);
        @(posedge clk) dmi_req_valid <= '0;
        wait (dmi_resp_valid == 1);
        @(posedge clk) dmi_resp_ready = 1;
        @(posedge clk) dmi_resp_ready = 0;
        //check dmstatus
        // $display("\ndmi resp %h\n",dmi_resp.data);
        if (resume_req && dmi_resp.data[17:16] === 2'b11) begin
          resume_req = 0;
          poll = 0;
          $display("[Poll] Clear Resume Req Poll");
          do_file_writes();
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
            core_haltg_hreq = 0;
            $display("[Poll] All cores in resume group are resumed");
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
        end else if(halted_sdtrig && dmi_resp.data[9:8] === 'h3)begin
          halted_sdtrig = 0;
          poll = 0;
          $display("[Poll] Clearing halted_sdtrig = 0");
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
