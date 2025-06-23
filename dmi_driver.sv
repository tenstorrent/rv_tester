`ifndef DMI_TB_WRITES_UNSUPPORTED
module dmi_driver 
import rv_tester_params:: * ;
(
    input logic                     clk,
    input logic                     core_clk,
    input logic                     reset_n,
    input logic                     warm_reset_sdtrig,

    input logic                     dmi_driver_dbg_enable,
    input logic [31:0]              rand_dmi_driver_dly,
    input logic [31:0]              hart_enable_mask,
    input logic [31:0]              dm_single_step_count,
    input logic [31:0]              sdtrig_multitrigger,
    input logic [31:0]              num_dm_randpc,
    input logic [31:0]              num_dm_randload,
    input logic [31:0]              num_dm_randstore,
    input logic [31:0]              trigger_config,
    input logic                     priority_singlestep,
    input logic                     disable_haltpoll,
    input logic                     disable_abscmdpoll,
    input logic                     disable_triggerpoll,
    input logic                     terminate,
    input logic [31:0]              num_harts,
    input logic                     sdtrig_display,
    input logic                     nonexistent_hart,
    input logic [31:0]              axi_resp_hang_addr,
    input logic [31:0]              abscmd_hang_counter,

    
    input logic                     dmi_req_ready,
    input logic                     dmi_resp_valid,
    input rv_tester_pkg::dmi_resp_t dmi_resp,

    output logic                          dmi_req_valid,
    output rv_tester_pkg::dmi_req_t       dmi_req,
    output logic                          dmi_resp_ready,
    output logic                    [7:0] misc_signals,

    output logic                    dmi_status,
    output logic [31:0]             dmi_commands_in_queue,

    input  logic [7:0]               DM_DebugReq_Valids,
    input rv_tester_pkg::dm_write_t trickbox_dmi_write,
    input rv_tester_params::rvfi_t[TOTAL_NRETS-1: 0] rvfi
);

  // -----------------------------
  // DMI Stimulus
  // -----------------------------
  rv_tester_pkg::dmi_req_t  command;
  rv_tester_pkg::dmi_resp_t response;
  rv_tester_pkg::dmi_req_t  command_hg[2];

  rv_tester_pkg::dmi_req_t  command_queue [$];
  rv_tester_pkg::dmi_resp_t response_queue[$];

  rv_tester_pkg::dmi_req_t single_step_ahead_command_queue[$], single_step_quit_command_queue[$], sdtrig_debug_mode_entry_queue[$], sdtrig_trigger_command_queue[$], sdtrig_trigger_disable_command_queue[$], sdtrig_progbuf_queue[$];
  rv_tester_pkg::dmi_req_t copy_sdtrig_progbuf_queue[$], copy_sdtrig_debug_mode_entry_queue[$], copy_sdtrig_trigger_command_queue[$], copy_sdtrig_trigger_disable_command_queue[$];

  logic command_trigger, response_trigger;
  logic terminate_d1, terminate_align;
  logic [31:0] clk_cnt = '0;
  logic halt_req, resume_req, abstr_cmd_req, poll, poll_p2, ndm_reset_init, ndm_reset_ack, ndm_reset_assert_done, ndm_reset_priority, poll_reset_completion, ndmreset_halt_req;
  logic [31:0] ext_trig_delay;
  logic [31:0] single_step_instr_cnt, single_step_executed_cnt;
  logic [31:0] ahead_queue_cnt, quit_queue_cnt, cnt;
  logic single_step_started, single_step_quit;
  logic abs_read, abs_write, abs_read_data, sdtrig_fire, halted_sdtrig;
  logic cores_in_halt_group, core_haltg_hreq, cores_in_resume_grp, core_resumeg_rreq, ack_havereset, remove_core_from_haltg, remove_core_from_resumeg;
  logic [7:0] core_in_halt_group, core_in_resume_grp, core_halted, core_resumed, core_ignore_hreq, core_ignore_rreq, core_disabled;
  logic [9:0] dm_hartsel, core_rg_check, core_ignore_resumepoll;
  logic [2:0] core_halt_index, core_resume_index;
  logic tdata1_read, tdata1_write, check_trigger_type, mcontrol6_trigger, trigger_to_fire, trigger_fired_halted, check_cause_trigger, cause_trigger, to_check_cause;
  logic dcsr_abscmd, dcsr_read, ss_step_bit, core_to_halt_after_ss, core_halted_after_ss;
  logic check_step_core, core_hg_resumed_ss, core_haltsum_ss, core_check_haltsum_ss;
  logic [2:0] core_ss_index, core_halted_ss, core_halted_sdtrig;
  logic tselect_core, tselect_core_complete, core_rg_halt_sdtrig, core_haltsum_sdtrig, check_haltsum_sdtrig;
  logic check_hartsellen, check_dmstatus_disc, hart_discovery, dmcontrol_hartsel;
  logic expect_cmd_err_excp, exception_illegal, read_cmisa_sdtrig, check_cmisa_sdtrig, cmisa_sdtrig_disabled;

  logic rvfi_sdtrig, disable_mem_access_checker, sdtrig_progbuf_exec, read_hartsel;
  logic [7:0] rvfi_sdtrig_core, rvfi_sdtrig_core_clr, cause_event_latched, cause_event_sync, cause_event_ff1, cause_event_ff2;
  int file_descr, count_hart_enable_mask, dmi_command_in_step_ahead_queue_size, dmi_command_in_step_quit_queue_size, single_step_instr_cnt_plusarg, total_triggers_plusarg,num_dm_randpc_plsg, num_dm_randload_plsg, num_dm_randstore_plsg, tselect_conf_plusarg, multitriggers_plusarg;
  int trigger_counter, total_command_in_sdtrig_trigger_queue_size;
  int trigger_index, total_command_in_sdtrig_progbuf_queue_size;
  logic check_hit_for_tselect, to_check_tselect, read_tselect, to_check_hit, check_hit_bit, read_tdata1_hit;

  logic mmr_write_32bits, mmr_write_64bits, check_data0, check_data1, get_data1, mmr_read_32bits, mmr_read_64bits, mmr_access_rd, read_data1, read_data0_comp, read_data1_comp;
  logic ss_ndmreset, modify_hartsel;
  logic read_data2, read_data3, get_data2, get_data3, end_of_test_cleanup;
  int data0_value, data1_value, hart_enable_mask_value, data2_value, data3_value, core_id_hit, core_hartsel_hit;
  int abscmd_counter;
  logic [7:0] DM_DebugReq_Valids_q;
  typedef struct packed {
    logic [15:0] reg_addr;
    logic [63:0] reg_data;
  } abs_reg_out;

  logic [7:0] trigger_hit [8] = '{default: 8'd0};
  logic [7:0] trigger_hit_chk [8] = '{default: 8'd0};

  abs_reg_out abs_data_temp_packet, abs_reg_out_queue[$];

  task reset_cleanup(); 
    begin
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
      tdata1_read <= 0;
      check_trigger_type <= 0;
      mcontrol6_trigger <= 0;
      trigger_to_fire <= 0;
      trigger_fired_halted <= 0;
      check_cause_trigger <= 0;
      cause_trigger <=0;
      to_check_cause <= 0;
      dcsr_abscmd <= 0;
      dcsr_read <= 0;
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
      dmi_command_in_step_ahead_queue_size <= 0;
      dmi_command_in_step_quit_queue_size <= 0;
      check_hit_for_tselect <= 0;
      to_check_tselect <= 0;
      read_tselect <= 0;
      to_check_hit <= 0;
      check_hit_bit <= 0;
      read_tdata1_hit <= 0;
      mmr_write_32bits <= 0;
      mmr_write_64bits <= 0;
      check_data0 <= 0;
      check_data1 <= 0;
      get_data1 <= 0;
      mmr_read_32bits <= 0;
      mmr_read_64bits <= 0;
      mmr_access_rd <= 0;
      read_data1 <= 0;
      read_data0_comp <= 0;
      read_data1_comp <= 0;
      dm_hartsel <= 0;
      core_disabled <= 0;
      hart_enable_mask_value <= 1;
      read_data2 <= 0;
      read_data3 <= 0;
      get_data2 <= 0;
      get_data3 <= 0;
      expect_cmd_err_excp <= 0;
      exception_illegal <= 0;
      read_cmisa_sdtrig <= 0;
      check_cmisa_sdtrig <= 0;
      cmisa_sdtrig_disabled <= 0;
      disable_mem_access_checker <= 0;
      ss_ndmreset <= 0;
      end_of_test_cleanup <= 0;
      core_ignore_resumepoll <= 0;
      terminate_d1 <= 0;
      terminate_align <= 0;
      tdata1_write <= 0;
      sdtrig_progbuf_exec <= 0;
      read_hartsel <= 0;
      modify_hartsel <= 0;
      rvfi_sdtrig_core_clr <= 0;
      total_command_in_sdtrig_trigger_queue_size <= 0;

      command_queue.delete();
      response_queue.delete();
      single_step_ahead_command_queue.delete();
      single_step_quit_command_queue.delete();
      sdtrig_debug_mode_entry_queue.delete();
      sdtrig_trigger_command_queue.delete();
      sdtrig_trigger_disable_command_queue.delete();
      sdtrig_progbuf_queue.delete();
      copy_sdtrig_progbuf_queue.delete();
      copy_sdtrig_debug_mode_entry_queue.delete();
      copy_sdtrig_trigger_command_queue.delete();
      copy_sdtrig_trigger_disable_command_queue.delete();

      $display("[DMI Driver] Reset State Cleaned-up \n");
    end
  endtask : reset_cleanup 
      
  assign multitriggers_plusarg = sdtrig_multitrigger;
  assign single_step_instr_cnt_plusarg = dm_single_step_count;
  assign misc_signals[0] = poll;
  assign dmi_status = poll;
  assign num_dm_randpc_plsg = num_dm_randpc;
  assign num_dm_randload_plsg = num_dm_randload;
  assign num_dm_randstore_plsg = num_dm_randstore;
  assign tselect_conf_plusarg = trigger_config;
  assign count_hart_enable_mask = $countones(hart_enable_mask);
  assign total_triggers_plusarg = num_dm_randpc_plsg + num_dm_randload_plsg + num_dm_randstore_plsg; // multiply with num harts
  //assign trigger_counter = (num_dm_randpc_plsg + num_dm_randload_plsg + num_dm_randstore_plsg)*num_harts;


  initial begin
    reset_cleanup();
  end

  always @(posedge clk) begin
    dmi_commands_in_queue = command_queue.size();

    if ((~reset_n || end_of_test_cleanup) || (~warm_reset_sdtrig && (trigger_config != 0))) begin
      reset_cleanup();
    end
  end

  always @(posedge clk) begin
    if (~reset_n) begin
      terminate_align <= '0;
      terminate_d1 <= '0;
    end
    else begin
      terminate_align <= terminate;
      terminate_d1 <= terminate_align;
    
      if(trigger_config != 0 && (terminate_align && ~terminate_d1) && dmi_driver_dbg_enable) begin
        for(int core_id=0; core_id<num_harts; core_id++) begin
          for (int tselect=0; tselect<8; tselect+=2) begin
            if(trigger_config[tselect] && trigger_config[tselect+1]) begin
              $display("Checking trigger pair [%0d, %0d]", tselect, tselect+1);
              if(trigger_hit_chk[core_id][tselect] || trigger_hit_chk[core_id][tselect+1]) begin
                $display("Trigger pair trigger_hit_chk[%0d][%0d]=%0d, trigger_hit_chk[%0d][%0d]=%0d are hit", core_id, tselect, trigger_hit_chk[core_id][tselect], core_id, tselect+1, trigger_hit_chk[core_id][tselect+1]);
              end else begin
                $error("Trigger pair trigger_hit_chk[%0d][%0d]=%0d, trigger_hit_chk[%0d][%0d]=%0d was configured but neither of the triggers are hit",core_id, tselect, trigger_hit_chk[core_id][tselect],core_id, tselect+1, trigger_hit_chk[core_id][tselect+1]);
              end
            end
          end
        end
      end
    end
  end

  always @(posedge clk or negedge clk) begin //sync the clk and use just posedge
    if(abstr_cmd_req && rvfi[0].cause[63:0] === 'h2) begin
      exception_illegal = 1;
      $display("[DMI Driver] Exception:2 is seen while executing an abs_cmd, hence setting exception_illegal");
    end else if(exception_illegal) begin
      exception_illegal = 0;
      $display("[DMI Driver] Exception != 2 during abs_cmd execution, hence clearing exception_illegal");
    end
  end

  //CDC synchronizer between fb_clk and core_clk.
  //As rvfi runs on core_clk and driver runs on fb_clk we would miss the exceptions, hence clk sync is needed.

  //Flop the cause_event
  always_ff @(posedge core_clk) begin
    if (!reset_n) begin //---> warm_reset_sdtrig
      for (int i = 0; i < num_harts; i++) begin
          cause_event_latched <= 0;
      end
    end else begin
      for (int core_id = 0; core_id < num_harts; core_id++) begin
        if(cause_event_latched[core_id] == cause_event_sync[core_id])
          cause_event_latched[core_id] <= (rvfi[core_id*8].cause[63:0] == 'h21);
      end
    end
  end

  //Flop the latched event
  always_ff @(posedge clk) begin
    if (!reset_n) begin //---> warm_reset_sdtrig
      for (int i = 0; i < num_harts; i++) begin
          cause_event_ff1 <= 0;
          cause_event_ff2 <= 0;
          cause_event_sync <= 0;
      end
    end else begin
      for (int i = 0; i < num_harts; i++) begin
          cause_event_ff1[i] <= cause_event_latched[i];
          cause_event_ff2[i] <= cause_event_ff1[i];
          cause_event_sync[i] <= cause_event_ff2[i];
      end
    end
  end

  always_ff @(posedge clk) begin
    if (!reset_n) begin //---> warm_reset_sdtrig
      rvfi_sdtrig_core <= '0;
      core_id_hit      <= '0;
      rvfi_sdtrig      <= '0;
    end else begin
      for (int i = 0; i < num_harts; i++) begin
        if (cause_event_sync[i]) begin
          rvfi_sdtrig_core[i] <= 1;
          core_id_hit[i]      <= 1;
          $display("[DMI Driver] core:%0d cause=0x21(excp:33) hit", i);
        end else if (rvfi_sdtrig_core_clr[i]) begin
          rvfi_sdtrig_core[i] <= 0;
          core_id_hit[i] <= 0;
        end
      end
      rvfi_sdtrig <= |rvfi_sdtrig_core;
    end
  end

  always @(posedge clk) begin
      if (!reset_n)
        DM_DebugReq_Valids_q <= 0;
      else
        DM_DebugReq_Valids_q <= DM_DebugReq_Valids;
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
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00011) begin
      sdtrig_debug_mode_entry_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the sdtrig debug entry queue, size=%h",
               sdtrig_debug_mode_entry_queue.size());
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00101) begin
      sdtrig_trigger_command_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the sdtrig trigger command queue, size=%h",
               sdtrig_trigger_command_queue.size());
      total_command_in_sdtrig_trigger_queue_size = sdtrig_trigger_command_queue.size();
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00111) begin
      sdtrig_trigger_disable_command_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the sdtrig trigger disable command queue, size=%h",
               sdtrig_trigger_disable_command_queue.size());
    end else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00110) begin
      sdtrig_progbuf_queue.push_back(
          rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
      $display("[DMI Driver] Added this command to the sdtrig program buffer queue, size=%h",
               sdtrig_progbuf_queue.size());
    end
  end

  always @(negedge clk) begin
    if (~reset_n)
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
      if (dmi_driver_dbg_enable) begin
        //decode request type
        if (cmd.addr === 'h10 && cmd.op === 'h2 && sdtrig_fire === 'h1) begin
          $display("[Poll] Check if the core is halted through sdtrig");
          sdtrig_fire = 0;
          halted_sdtrig = 1;
          poll = 1;
          $display("[Poll] Clearing sdtrig_fire = 0");
          $display("[Poll] Setting halted_sdtrig = 1");
        end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[31] === '1 && cmd.data[1] === '0) begin
          dm_hartsel = cmd.data[25:16];
          if(~core_in_halt_group[dm_hartsel]) begin
            $display("[Poll] Seen Halt Req, Doing Poll");
            if(~disable_haltpoll) begin
              halt_req = 1;
              poll = 1;
            end else begin
              core_ignore_resumepoll = dm_hartsel;
              $display("[DMI Driver] core_ignore_resumepoll=%h , dm_hartsel=%h", core_ignore_resumepoll, dm_hartsel);
              $display("[DMI Driver] disable_haltpoll is set and ignore haltreq poll");
            end
          end else begin
            $display("[Poll] Core in halt group gets a haltreq, Doing Poll");
            core_haltg_hreq = 1;
            poll = 1;
          end
        end else if(nonexistent_hart && cmd.addr === 'h10 && cmd.op === 'h1) begin
          read_hartsel = 1;
          poll = 1;
          $display("[Poll] Poll for core0 to be halted");
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
          if(cmd.data[1] === 'h1) begin
            ss_ndmreset = 1;
            $display("[Poll] SS_Ndmreset is set");
          end
        end else if (cmd.addr === 'h17 && cmd.op === 'h2 && ~disable_abscmdpoll) begin
          $display("[Poll] Seen Abstract Command Req, Doing Poll");
          abstr_cmd_req = 1;
          poll = 1;
          $display("[Poll] #310 cmd.addr = :%h, cmd.op = :%h, cmd.data[31:0]= :%h", cmd.addr, cmd.op, cmd.data[31:0]);
          if(cmd.data[31:0] === 'h2b10000)begin
            mmr_write_64bits = 1;
            $display("[Poll] MMR Access - 64 bits Write");
          end else if(cmd.data[31:0] === 'h2a10000)begin
            mmr_write_32bits = 1;
            $display("[Poll] MMR Access - 32 bits Write");
          end else if(((cmd.data[31:0] === 'h2b00000) && mmr_read_64bits) || ((cmd.data[31:0] === 'h2a00000) && mmr_read_32bits)) begin
            mmr_access_rd = 1;
            $display("[Poll] mmr_access_rd is set");
          end
          if (cmd.data[31:24] === 'h0 && cmd.data[17] === 'h1) begin
            //Seen an abstract reg command with rd/write
            $display("[Poll] Seen an abstract command with rd/write");
            if (cmd.data[16] === 'h1) begin
              $display("[Poll] Seen the abstract command with write");
              abs_write = 1;
              if(cmd.data[15:0] === 'h07a1) begin
                $display("[sdtrig:Poll] Seen an abstract command write on tdata1");
                tdata1_write = 1;
              end else if(cmd.data[15:0] === 'h07a0 && (trigger_config == 0))begin
                tselect_core = 1;
                $display("[sdtrig:Poll] Check which core has sdtrig configurations");
              end
            end else if (cmd.data[16] === 'h0) begin
              $display("[Poll] Seen the abstract command with read");
              abs_read = 1;
              abs_data_temp_packet.reg_addr = cmd.data[15:0];
              if(to_check_cause && cmd.data[15:0] === 'h7b0) begin
                to_check_cause = 0;
                check_cause_trigger = 1;
                $display("[Poll] check_cause_trigger is set");
              end else if(to_check_hit && cmd.data[15:0] === 'h07a1) begin
                to_check_hit = 0;
                if(trigger_config != 0)
                  check_hit_bit = 1;
                $display("[Poll] check_hit_bit is set");
              end else if(cmd.data[15:0] === 'h07a1) begin
                $display("[sdtrig:Poll] Seen an abstract command read on tdata1");
                tdata1_read = 1;
              end else if (cmd.data[15:0] === 'h07b0) begin
                $display("[Poll] Seen the abstract command read on dcsr");
                dcsr_abscmd = 1;
              end else if(cmd.data[15:0] === 'h0bcc) begin
                check_cmisa_sdtrig = 1;
                $display("[Poll] check_cmisa_sdtrig is set in #412");
              end else if(cmisa_sdtrig_disabled && (cmd.data[15:3] === 'h0f4)) begin
                expect_cmd_err_excp = 1;
                $display("[Poll] expect_cmd_err_excp is set in #415");
              end
            end
          end
        end else if (cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[1] === 'h1 ) begin
          $display("[Poll] Making NdmReset = 1, Doing Poll");
          ndm_reset_init = 1;
          ndm_reset_assert_done = 1;
          poll = 1;
          trigger_to_fire = 0;
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
          dcsr_read = 1;
          poll = 1;
        end else if(core_to_halt_after_ss && cmd.addr === 'h11 && cmd.op === 'h1 && ~trigger_to_fire) begin
          $display("[Single step] Core resuming after step configuration");
          poll = 1;
        end else if(core_to_halt_after_ss && cmd.addr === 'h11 && cmd.op === 'h1 && trigger_to_fire && priority_singlestep) begin
          $display("[Single step] Core resuming single step x sdtrig, single step takes priority");
          trigger_to_fire = 0;
          poll = 1;          
        end else if(cmd.addr === 'h16 && cmd.op === 'h1 && (tdata1_read || tdata1_write)) begin
          $display("trigger type is configured in tdata1");
          tdata1_read = 0;
          tdata1_write = 0;
          check_trigger_type = 1;
          poll = 1;
        end else if(cmd.addr === 'h16 && cmd.op === 'h1 && tselect_core) begin
          $display("trigger type is configured in tdata1");
          tselect_core = 0;
          tselect_core_complete = 1;
          poll = 1;
        end else if(cmd.addr === 'h16 && cmd.op === 'h1 && (mmr_write_32bits || mmr_write_64bits))begin
          mmr_write_32bits = 0;
          check_data0 = 1;
          poll = 1;
          $display("Check data0 write value");
        end else if(trigger_to_fire && cmd.addr === 'h11 && cmd.op === 'h1 && ~priority_singlestep) begin
          $display("[Sdtrig] Core resuming after sdtrig configuration");
          if(!rvfi_sdtrig) begin
            @(rvfi_sdtrig or negedge reset_n);
          end
          poll = 1;
          trigger_fired_halted = 1;
          trigger_to_fire = 0;
          if(core_to_halt_after_ss) begin
            core_to_halt_after_ss = 0;
            $display("[Sdtrig] Sdtrig takes priority over single stepping");
          end
        end else if(trigger_to_fire && cmd.addr === 'h10 && cmd.op === 'h2) begin
          trigger_fired_halted = 1;
          trigger_to_fire = 0;
          $display("[Sdtrig] Check if the selected hart is halted");
          poll = 1;
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
        end else if((check_hit_bit || disable_triggerpoll) && cmd.addr === 'h4 && cmd.op === 'h1) begin
          poll = 1;
          check_hit_bit = 0;
          read_tdata1_hit = 1;
        end else if(check_data1 && cmd.addr === 'h4 && cmd.op === 'h2) begin
          poll = 1;
          get_data1 = 1;
          check_data1 = 0;
          //check_data2 = 1;
          $display("get_data1:%h to compare", get_data1);
        end else if(mmr_access_rd && cmd.addr === 'h4 && cmd.op === 'h1) begin
          poll = 1;
          //read_data0_comp = 1;
          mmr_access_rd = 0;
          read_data2 = 1;
          $display("Read data0 to compare 32bit read");
        end else if(read_data1 && cmd.addr === 'h5 && cmd.op === 'h1) begin
          poll = 1;
          read_data1 = 0;
          read_data1_comp = 1;
          $display("Read data1 to compare 64bit read");          
        end else if(check_cmisa_sdtrig && cmd.addr === 'h4 && cmd.op === 'h1) begin
          poll = 1;
          check_cmisa_sdtrig = 0;
          read_cmisa_sdtrig = 1;
        end else if(disable_abscmdpoll && cmd.addr === 'h16 && cmd.op === 'h1) begin
          abstr_cmd_req = 1;
          poll = 1;
          $display("Check if abscmd is executed and busy is cleared");
        end
      end
    end
  endtask : is_poll_needed

  task do_polling();
    begin
      $display("[Poll] Starting poll for halt:%h resume:%h abstract:%h", halt_req, resume_req,
               abstr_cmd_req);
      for(int i=0; i<count_hart_enable_mask; i++) begin
        hart_enable_mask_value = hart_enable_mask_value | (1 << i);
      end
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
        end else if (dcsr_read) begin
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
        end else if (read_tdata1_hit) begin
          $display("[Poll] #534 read hit");
          dmi_req <= 41'h1100000000;
        end else if (check_data0) begin
          $display("[Poll] data0 write value for mem access");
          dmi_req <= 41'h1100000000;
        end else if(get_data1) begin
          $display("[Poll] data1 write value for mem access with size:64");
          dmi_req <= 41'h1500000000;
        end else if(read_data0_comp) begin
          $display("[Poll] data0 read value for mem access");
          dmi_req <= 41'h1100000000;
        end else if(read_data1_comp)begin
          $display("[Poll] data1 read value for mem access with size:64");
          dmi_req <= 41'h1500000000;
        end else if(get_data2 || read_data2) begin
          $display("[Poll] store/read the address from data2 to compare ");
          dmi_req <= 41'h1900000000;
        end else if(get_data3 || read_data3) begin
          $display("[Poll] store/read the address from data3 to compare ");
          dmi_req <= 41'h1d00000000;
        end else if(read_cmisa_sdtrig) begin
          $display("[Poll] data0 read value to check sdtrig field in cmisa csr");
          dmi_req <= 41'h1100000000;
        end else if (read_hartsel) begin
          $display("[Poll] Read dmcontrol to check hartsel to be 0");
          dmi_req <= 41'h4100000000;
        end
        wait (dmi_req_ready == 1);
        @(posedge clk) dmi_req_valid <= '0;
        wait (dmi_resp_valid == 1);
        @(posedge clk) dmi_resp_ready <= 1;
        @(posedge clk) dmi_resp_ready <= 0;
        //check dmstatus
        // $display("\ndmi resp %h\n",dmi_resp.data);
        if (resume_req) begin
          if(hart_enable_mask_value[dm_hartsel]) begin
            if(dmi_resp.data[17:16] === 2'b11 && ~ss_ndmreset) begin
              resume_req = 0;
              poll = 0;
              if(mcontrol6_trigger && ~disable_triggerpoll) begin
                trigger_to_fire = 1;
                mcontrol6_trigger = 0;
                $display("[Poll] trigger_to_fire is set");
              end
              $display("[Poll] Clear Resume Req Poll");
              do_file_writes();
              if(ss_step_bit && ~ss_ndmreset) begin
                core_to_halt_after_ss = 1;
                $display("core_to_halt_after_ss is set");
              end
              if(core_halted_after_ss) begin
                core_halted_after_ss = 0;
                $display("core_halted_after_ss is cleared");
              end
            end
            if(ss_ndmreset && dmi_resp.data[19:18] === 2'b11) begin
              ss_ndmreset = 0;
              resume_req = 0;
              ndm_reset_init =1;
              ndm_reset_assert_done = 1;
              $display("core is reset upon ndmreset assertion along with resumereq");
            end
          end else begin
            poll = 0;
            resume_req = 0;
            $display("[Poll] Core disabled - Not polling for resume req");
          end
        end else if (halt_req)begin
          if(hart_enable_mask_value[dm_hartsel]) begin
            $display("[Poll] Selected hart:%h is enabled and expected to halt", dm_hartsel);
            if(dmi_resp.data[9:8] === 2'b11) begin
              $display("[Poll] Selected hart:%h is enabled and halted", dm_hartsel);
              halt_req = 0;
              poll = 0;
              $display("[Poll] Clear Halt Req Poll");
            end
          end else if (!hart_enable_mask_value[dm_hartsel]) begin
            $display("[Poll] Selected hart:%h is disabled and should be unavailable", dm_hartsel);
            if(dmi_resp.data[13:12] === 2'b11) begin
              $display("[Poll] Selected hart:%h is disabled and is unavailable", dm_hartsel);
              halt_req = 0;
              poll = 0;
              $display("[Poll] Clear Halt Req Poll");
            end
          end
        end else if (read_hartsel) begin
          dm_hartsel = dmi_resp.data[25:16];
          $display("[Poll] read dm_hartsel: %0d after sending haltreq to core8", dm_hartsel);
          modify_hartsel = 1;
          poll = 0;
        end else if (abstr_cmd_req) begin
          if(hart_enable_mask_value[dm_hartsel]) begin
            $display("[Poll] Selected hart is enabled and busy should be cleared");
            if(dmi_resp.data[12] === 1'b0) begin
              $display("[Poll] Selected hart is enabled and busy is cleared");
              abstr_cmd_req = 0;
              $display("[Poll] Clear Abstract Command Req Poll");
              $display("[Poll] Reading the data output of the abstract command");
              if (abs_read === 'h1) begin
                abs_read_data = 1;
              end else begin
                poll = 0;
              end
            end else if(axi_resp_hang_addr) begin
              if(abscmd_counter == abscmd_hang_counter)begin
                abstr_cmd_req = 0;
                poll = 0;
                $display("[Poll] Clearing abc cmd poll as the core is hung; abscmd_counter: %0d", abscmd_counter);
              end else begin
                abscmd_counter += 1;
                $display("[Poll] Decrementing abscmd_counter: %0d", abscmd_counter);
              end
            end
            if(expect_cmd_err_excp)begin
              $display("[Poll] wait for Illegal exception");
              @(exception_illegal);
              $display("[Poll] Illegal exception is seen");
              expect_cmd_err_excp = 0;
            end
            if(dmi_resp.data[10:8] === 2'b11) begin
              disable_mem_access_checker = 1;
              $display("[Poll] Setting disable_mem_access_checker");
            end
          end else if (!hart_enable_mask_value[dm_hartsel]) begin
            $display("[Poll] Selected hart:%h is disabled and expect cmderr=4", dm_hartsel);
            if(dmi_resp.data[10] === 1'b1) begin
              abstr_cmd_req = 0;
              $display("[Poll] Selected hart:%h is disabled and cmderr=4", dm_hartsel);
              poll = 0;
            end
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
            //if(core_in_halt_group[ii] && dmi_resp.data[ii])begin
            if(core_in_halt_group[ii]) begin
              if(dmi_resp.data[ii] && hart_enable_mask_value[ii])begin
                core_halted[ii] = 1;
                $display("[Poll] core%0d in halt group is halted", ii);
              end else if((dmi_resp.data[ii] || hart_enable_mask_value[ii]) === 0) begin
                core_disabled[ii] = 1;
                $display("[Poll] core%0d is disabled", ii);
              end
            end else if(!core_in_halt_group[ii]) begin
              core_ignore_hreq[ii] = 1;
              $display("[Poll] core%0d is ignored as it's not part of halt group", ii);
            end
          end
          if((core_halted[0] || core_ignore_hreq[0] || core_disabled[0]) && (core_halted[1] || core_ignore_hreq[1]|| core_disabled[1]) &&
          (core_halted[2] || core_ignore_hreq[2] || core_disabled[2]) && (core_halted[3] || core_ignore_hreq[3] || core_disabled[3]) && (core_halted[4] || core_ignore_hreq[4] || core_disabled[4]) &&
          (core_halted[5] || core_ignore_hreq[5] || core_disabled[5]) && (core_halted[6] || core_ignore_hreq[6] || core_disabled[6]) && (core_halted[7] || core_ignore_hreq[7] || core_disabled[7])) begin
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
            //if(core_in_resume_grp[ii] && ~dmi_resp.data[ii])begin
            if(core_in_resume_grp[ii])begin
              //ii==0 to satisfy the condition for core0, since core_ignore_resumepoll is 0 by defaulf.
              if(hart_enable_mask_value[ii] && ~dmi_resp.data[ii] && ((core_ignore_resumepoll != ii) | ii==0))begin
                core_resumed[ii] = 1;
                $display("[Poll] core%0d in resume group is resumed", ii);
              end else if ((hart_enable_mask_value[ii] || dmi_resp.data[ii]) == 0) begin
                core_disabled[ii] = 1;
                $display("[Poll] core%0d is disabled", ii);
              end
              // As core_ignore_resumepoll would be 0 by default, ii!=0 is added to gaurd it from executing for all hartgroup tests.
              // should only satisfy when core_ignore_resumepoll == 2, a non zero value
              if((core_ignore_resumepoll == ii) & ii!=0) begin
                core_ignore_rreq[ii] = 1;
                $display("[Poll] core%0d is ignored as it receives resumereq while halting", ii);
              end
            end else if(!core_in_resume_grp[ii]) begin
              core_ignore_rreq[ii] = 1;
              $display("[Poll] core%0d is ignored as it's not part of resume group", ii);
            end
          end
          if((core_resumed[0] || core_ignore_rreq[0] || core_disabled[0]) && (core_resumed[1] || core_ignore_rreq[1] || core_disabled[1]) &&
          (core_resumed[2] || core_ignore_rreq[2] || core_disabled[2]) && (core_resumed[3] || core_ignore_rreq[3] || core_disabled[3]) && (core_resumed[4] || core_ignore_rreq[4] || core_disabled[4]) &&
          (core_resumed[5] || core_ignore_rreq[5] || core_disabled[5]) && (core_resumed[6] || core_ignore_rreq[6] || core_disabled[6]) && (core_resumed[7] || core_ignore_rreq[7] || core_disabled[7])) begin
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
        end else if(dcsr_read) begin
          if(dmi_resp.data[2] === 'h1) begin
            ss_step_bit = 1;
            $display("[Poll] step bit is set in dcsr");
          end else if(dmi_resp.data[2] === 'h0 && ss_step_bit) begin
            ss_step_bit = 0;
            $display("[Poll] step bit is cleared in dcsr");
          end
          poll = 0;
          dcsr_read = 0;
          $display("[Poll] dcsr_read polling completed");
        end else if(core_to_halt_after_ss && dmi_resp.data[9:8] === 2'b11) begin
          core_to_halt_after_ss = 0;
          core_halted_after_ss = 1;
          poll = 0;
          $display("[Poll] core_halted_after_ss is set");
        end else if(check_trigger_type) begin
          if(dmi_resp.data[31:28] === 'h6) begin
            mcontrol6_trigger = 1;
          end else if (dmi_resp.data[31:28] === 'h0 || 'hf) begin
            mcontrol6_trigger = 0;
          end
          check_trigger_type = 0;
          poll = 0;
        end else if(trigger_fired_halted && dmi_resp.data[9:8] === 'h3)begin
          trigger_fired_halted =0;
          to_check_hit = 1;
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
          if(hart_enable_mask_value[dmcontrol_hartsel] === 1 && dmi_resp.data[11:10] === 3)  begin
            hart_discovery = 0;
            poll = 0;
            dmcontrol_hartsel = 0;
            $display("Selected hart is running as it's enabled");
          end else if(hart_enable_mask_value[dmcontrol_hartsel] === 0 && dmi_resp.data[13:12] === 3 && dmi_resp.data[11:10] === 0) begin
            hart_discovery = 0;
            poll = 0;
            dmcontrol_hartsel = 0;
            $display("Selected hart is unavailable as it's disabled");
          end
          if(dmcontrol_hartsel === 7) begin
            check_dmstatus_disc = 0;
            $display("Clear check_dmstatus_disc");
          end
        end else if(read_tdata1_hit) begin
          read_tdata1_hit = 0;
          if(disable_triggerpoll)begin
            if(~dmi_resp.data[22])begin
              $display("hit is zero as trigger shouldn't be fired in Debug Mode");
              poll = 0;
            end
          end else begin
            if(dmi_resp.data[22])begin
              trigger_hit[core_hartsel_hit][trigger_index]= 1;
              trigger_hit_chk[core_hartsel_hit][trigger_index] = 1;
              $display("hit is set for core:%0d teslect:%h", core_hartsel_hit, trigger_index);
            end else begin
              $display("hit not set for core:%0d teslect:%h", core_hartsel_hit, trigger_index);
            end
            if(trigger_index < 7) begin
              to_check_hit = 1;
              $display("to_check_hit is set for core:%0d tselect:%h", core_hartsel_hit, trigger_index);
            end else begin
              to_check_hit = 0;
              $display("to_check_hit is cleared for core:%0d tselect:%h", core_hartsel_hit, trigger_index);
            end
            poll = 0;
          end
        end else if(check_data0) begin
          data0_value = dmi_resp.data;
          $display("data0_value:%h", data0_value);
          if(mmr_write_64bits) begin
            check_data1 = 1;
            mmr_write_64bits = 0;
            mmr_read_64bits = 1;
            $display("Check data1 as it's a 64 bit write");
          end else begin
            mmr_read_32bits = 1;
            $display("mmr_read_32bits is set");
          end
            check_data0 = 0;
          get_data2 = 1;
        end else if(get_data2)begin
          data2_value = dmi_resp.data;
          $display("data2_value:%h", data2_value);
          get_data2 = 0;
          get_data3 = 1;
        end else if(get_data3)begin
          data3_value = dmi_resp.data;
          $display("data3_value:%h", data3_value);
          get_data3 = 0;
            poll = 0;
        end else if(get_data1) begin
          data1_value = dmi_resp.data;
          $display("data1_value:%h", data1_value);
          poll = 0;
          get_data1 = 0;
        end else if(read_data2) begin
          $display("data2_stored_value:%h data2_read_value:%h", data2_value, dmi_resp.data);
          if(data2_value === dmi_resp.data) begin
            read_data2 = 0;
            read_data3 = 1;
            $display("read_data3 is set");
          end else begin
            poll = 0;
            read_data2 = 0;
            $display("read_data2: read is not happening for the written addr");
          end
        end else if(read_data3) begin
          $display("data3_stored_value:%h data3_read_value:%h", data3_value, dmi_resp.data);
          if(data3_value === dmi_resp.data) begin
            read_data3 = 0;
            read_data0_comp = 1;
            $display("read_data0_comp is set");
          end else begin
            poll = 0;
            read_data3 = 0;
            $display("read_data3: read is not happening for the written addr");
          end
        end else if(read_data0_comp) begin
          $display("line #889 data0_value:%h, dmi_resp.data:%h", data0_value, dmi_resp.data);
          if(!disable_mem_access_checker) begin
            if(data0_value === dmi_resp.data) begin
              $display("data0_value:%h, dmi_resp.data:%h", data0_value, dmi_resp.data);
              poll = 0;
              read_data0_comp = 0;
              mmr_read_32bits = 0;
              if(mmr_read_64bits) begin
                read_data1 = 1;
                mmr_read_64bits = 0;
                $display("read_data1 is set");
              end
            end else begin
              $display("Error: Mismatch scratchpad_mmr_write_data0_value: %h, scratchpad_mmr_read_data0_value: %h", data0_value, dmi_resp.data);
            end
          end else begin
            $display("Mem Access checker is disabled");
            poll = 0;
            read_data0_comp = 0;
            mmr_read_32bits = 0;
          end
          disable_mem_access_checker = 0;
        end else if(read_data1_comp) begin
          if(data1_value === dmi_resp.data) begin
            $display("data1_value:%h, dmi_resp.data:%h", data1_value, dmi_resp.data);
            poll = 0;
            read_data1_comp = 0;
          end else begin
            $display("Error: Mismatch scratchpad_mmr_write_data1_value: %h, scratchpad_mmr_read_data1_value: %h", data1_value, dmi_resp.data);
          end
        end else if(read_cmisa_sdtrig) begin
          if(dmi_resp.data[23] === 0) begin
            cmisa_sdtrig_disabled = 1;
            $display("cmisa_sdtrig_disabled is set");
          end
          poll = 0;
          read_cmisa_sdtrig = 0;
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
        if(trigger_config > 0 && (command_queue.size() == 3)) begin
          command_hg[1] = command_queue.pop_front();
          for(int ii=0; ii<num_harts; ii++)begin
            command_hg[0] = command;
            command_hg[0].data[25:16] = ii;
            $display("[DMI Execution] executing command_hg");
            foreach (command_hg[i]) begin
              drive_dmi_cmd(command_hg[i]);
              is_poll_needed(command_hg[i]);
              if (poll) begin
                do_polling();
              end
            end
          end
          command = command_queue.pop_front();
        end
        $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                 command.data);
        if(modify_hartsel) begin
          command.data[25:16] = dm_hartsel;
          modify_hartsel = 0;
          $display("[DMI Execution] Modified dmcontrol after selecting nonexistent core ==> data:%h", command.data);
        end
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

      if(core_to_halt_after_ss) begin
        for(int step_count=single_step_instr_cnt_plusarg; step_count>0; step_count--)begin
          if (step_count != 1) begin
            dmi_command_in_step_ahead_queue_size = single_step_ahead_command_queue.size();
            $display("[DMI Driver] Added this command to the single step (ahead) queue, size=%h",
                    single_step_ahead_command_queue.size());
            while(dmi_command_in_step_ahead_queue_size > 0) begin
              command = single_step_ahead_command_queue.pop_front();
              drive_dmi_cmd(command);
              is_poll_needed(command);
              if (poll) begin
                do_polling();
              end
              dmi_command_in_step_ahead_queue_size--;
              single_step_ahead_command_queue.push_back(command);
            end
          end
          else begin
            dmi_command_in_step_quit_queue_size = single_step_quit_command_queue.size();
            while(dmi_command_in_step_quit_queue_size > 0) begin
              command = single_step_quit_command_queue.pop_front();
              drive_dmi_cmd(command);
              is_poll_needed(command);
              if (poll) begin
                do_polling();
              end
              dmi_command_in_step_quit_queue_size--;
              single_step_quit_command_queue.push_back(command);
            end
          end
        end
      end

      //Configure Dmode, Action and Trigger type for all the triggers
      
      total_command_in_sdtrig_progbuf_queue_size = sdtrig_progbuf_queue.size();
      if(total_triggers_plusarg > 0)begin
        trigger_counter = total_triggers_plusarg * num_harts;
        sdtrig_progbuf_exec = 1;
        for(int num_core=0; num_core<num_harts; num_core++) begin
          $display("Executing from sdtrig prog buff queues core:%0d", num_core);
          for(int trigger_count=0; trigger_count<8; trigger_count++)begin     
            //----> if(trigger_config[trigger_count]==1);   
            copy_sdtrig_progbuf_queue = sdtrig_progbuf_queue;
            $display("Itrerating through for trigger_count:%h", trigger_count);
            while(copy_sdtrig_progbuf_queue.size()> 0) begin
              $display("Executing the while from sdtrig prog buff queues");
              //----take a copy of the queue
              if(copy_sdtrig_progbuf_queue.size() === (total_command_in_sdtrig_progbuf_queue_size)) begin
                command = copy_sdtrig_progbuf_queue.pop_front();
                command.data[25:16] = num_core;
              end else if(copy_sdtrig_progbuf_queue.size() === (total_command_in_sdtrig_progbuf_queue_size-2)) begin
                command = copy_sdtrig_progbuf_queue.pop_front();
                command.data[23:20] = trigger_count;
              end else if(copy_sdtrig_progbuf_queue.size() === 1) begin
                command = copy_sdtrig_progbuf_queue.pop_front();
                command.data[25:16] = num_core;
              end else begin
                command = copy_sdtrig_progbuf_queue.pop_front();
              end
              //Resume only after iterating program buffer for all tselect to configure Action, type & dmode
              if((tselect_conf_plusarg[trigger_count] === 1 && (copy_sdtrig_progbuf_queue.size()>0 || trigger_count === 7)) || (tselect_conf_plusarg[trigger_count] === 0 && trigger_count === 7 && copy_sdtrig_progbuf_queue.size()===0))begin // TODO: review
                $display("Executing from sdtrig_trigger_progbuf_queue");
                $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                    command.data);
                drive_dmi_cmd(command);
                is_poll_needed(command);
                if (poll) begin
                  do_polling();
                end
              end
            end
          end
        end
        sdtrig_progbuf_exec = 0;
        $display("[Poll] trigger configuration done for all cores");
      end
      command_trigger = 0;
      $display("[DMI Execution] Clear the Execution Trigger\n");
      if((command_queue.size() | dmi_command_in_step_quit_queue_size) == 0 && trigger_config==0) begin // guard with trigger_config
        end_of_test_cleanup = 1;
        $display("[DMI Driver] Initializing driver variables at End of Test");
      end
      @(posedge clk);
    end
  end

  always @(posedge rvfi_sdtrig) begin
    while(rvfi_sdtrig && trigger_config != 0) begin
      if(sdtrig_progbuf_exec)begin //fixme: use command_trigger
        $display("waiting for core to exit debug mode");
        repeat(5)
          @(posedge clk);
      end 
      else begin
        $display("Executing from sdtrig hit queues");
        $display("trigger_counter= %h", trigger_counter);
        for(int core_id=0; core_id < num_harts; core_id++) begin
          if(core_id_hit[core_id]) begin
            trigger_to_fire = 1;
            $display("trigger_to_fire is set after seeing exception: 33 for core:%0d", core_id);
            copy_sdtrig_debug_mode_entry_queue = sdtrig_debug_mode_entry_queue;
            while(copy_sdtrig_debug_mode_entry_queue.size() > 0) begin
              if(copy_sdtrig_debug_mode_entry_queue.size()==2) begin
                for(int tselect=0; tselect<8; tselect++)begin
                  if(tselect_conf_plusarg[tselect] === 1) begin
                    copy_sdtrig_trigger_command_queue = sdtrig_trigger_command_queue;
                    while(copy_sdtrig_trigger_command_queue.size() > 0) begin
                      if(copy_sdtrig_trigger_command_queue.size()===total_command_in_sdtrig_trigger_queue_size) begin
                        command = copy_sdtrig_trigger_command_queue.pop_front();
                        command.data = tselect;
                      end else begin
                        command = copy_sdtrig_trigger_command_queue.pop_front();
                      end
                      $display("Executing from sdtrig_trigger_command_queue");
                      $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                      command.data);
                      drive_dmi_cmd(command);
                      is_poll_needed(command);
                      if(read_tdata1_hit)begin
                        trigger_index = tselect;
                        core_hartsel_hit = core_id;
                        $display("Read hit for tselect:%0d, core: %0d", tselect, core_id);
                      end
                      if (poll) begin
                        do_polling();
                      end
                      $display("#1388 trigger_hit[core_id: %0d][tselect: %0d] = %0d",core_id, tselect,  trigger_hit[core_id][tselect]);
                    end
                    if(trigger_hit[core_id][tselect]) begin
                      to_check_cause = 1;
                      $display("Check for cause after hit");
                      copy_sdtrig_trigger_disable_command_queue = sdtrig_trigger_disable_command_queue;
                      while(copy_sdtrig_trigger_disable_command_queue.size()>0)begin
                        command = copy_sdtrig_trigger_disable_command_queue.pop_front();
                        $display("Executing from sdtrig_trigger_disable_command_queue");
                        $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                          command.data);
                        drive_dmi_cmd(command);
                        is_poll_needed(command);
                        if (poll) begin
                          do_polling();
                        end
                        trigger_hit[core_id][tselect] = 0;
                      end
                      trigger_counter--;
                    end
                  end
                end

                // Setting the rvfi_sdtrig_core_clr before resuming the core
                rvfi_sdtrig_core_clr[core_id] = 1;
                $display("clearing the rvfi_sdtrig_core for core: %0d", core_id);
                //Clearing rvfi_sdtrig_core_clr as it takes 1 clk cycle to clear rvfi_sdtrig_core
                @(posedge clk);
                  rvfi_sdtrig_core_clr[core_id] = 0;
                              
                command = copy_sdtrig_debug_mode_entry_queue.pop_front();
                $display("#973 Executing from sdtrig_debug_mode_entry_queue");
                command.data[25:16] = core_id;
                $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                  command.data);
                drive_dmi_cmd(command);
                is_poll_needed(command);
                if (poll) begin
                  do_polling();
                end
              end else if(copy_sdtrig_debug_mode_entry_queue.size()==1) begin
                command = copy_sdtrig_debug_mode_entry_queue.pop_front();
                if(trigger_counter==0) begin
                  $display("Executing from sdtrig_debug_mode_entry_queue");
                  $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                  command.data);
                  if (command.op == 'h3) begin
                    $display("[DMI Execution] Encountered Debug Checkopoint, Switching Control to Assembly");
                    break;
                  end
                end
              end else begin
                command = copy_sdtrig_debug_mode_entry_queue.pop_front();
                command.data[25:16] = core_id;
                $display("Executing from sdtrig_debug_mode_entry_queue");
                $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h", command.addr, command.op,
                  command.data);
                drive_dmi_cmd(command);
                is_poll_needed(command);
                if (poll) begin
                do_polling();
                  $display("polling in progress");
                end
                $display("Decrementing sdtrig_debug_mode_entry_queue");
              end
            end
          end
        end
        if(sdtrig_display)
          $display("#1457 Before posedge rvfi_sdtrig: %0d rvfi_sdtrig_core: %0d", rvfi_sdtrig, rvfi_sdtrig_core);
        @(posedge clk);
        if(sdtrig_display)
          $display("#1459 After posedge rvfi_sdtrig: %0d rvfi_sdtrig_core: %0d", rvfi_sdtrig, rvfi_sdtrig_core);
      end
      if(sdtrig_display)
        $display("Before posedge rvfi_sdtrig: %0d rvfi_sdtrig_core: %0d", rvfi_sdtrig, rvfi_sdtrig_core);
      @(posedge clk);
      if(sdtrig_display)
        $display("After posedge rvfi_sdtrig: %0d rvfi_sdtrig_core: %0d", rvfi_sdtrig, rvfi_sdtrig_core);
    end
    $display("Hit and cause for all the triggers that fired has been verified and triggers disabled");
    @(posedge clk);
  end

endmodule
`endif
