`ifndef DMI_TB_WRITES_UNSUPPORTED
module dmi_driver(
    input  logic                      clk,
    input  logic                      reset,
    input  logic                      dmi_req_ready,
    input  logic                      dmi_resp_valid,
    input  rv_tester_pkg::dmi_resp_t  dmi_resp,

    output logic                      dmi_req_valid,
    output rv_tester_pkg::dmi_req_t   dmi_req,
    output logic                      dmi_resp_ready,

    input  rv_tester_pkg::dm_write_t  trickbox_dmi_write
 );

    // always @(posedge clk) begin
    //   if(dmi_req_valid && dmi_req_ready) begin
    //     dmi_monitor_dpi(dmi_req.data,dmi_req.op,dmi_req.addr); //DPI-Call
    //   end
    // end

    // -----------------------------
    // DMI Stimulus
    // -----------------------------
    rv_tester_pkg::dmi_req_t command;
    rv_tester_pkg::dmi_resp_t response;

    rv_tester_pkg::dmi_req_t command_queue[$];
    rv_tester_pkg::dmi_resp_t response_queue[$];

    rv_tester_pkg::dmi_req_t single_step_ahead_command_queue[$], single_step_quit_command_queue[$];
    rv_tester_pkg::dmi_req_t single_step_ahead_command_queue_backup[$], single_step_quit_command_queue_backup[$];

    logic command_trigger, response_trigger;
    logic [31:0] clk_cnt;
    logic halt_req,resume_req,abstr_cmd_req,poll;
    logic [31:0] ext_trig_delay;
    logic [31:0] single_step_instr_cnt, single_step_executed_cnt;
    logic [31:0] ahead_queue_cnt, quit_queue_cnt, cnt;
    logic single_step_started, single_step_quit;
    logic abs_read, abs_write, abs_read_data;

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
        halt_req <=0;
        resume_req <=0;
        abstr_cmd_req  <=0;
        poll <=0;
        ext_trig_delay <= 0;
        single_step_executed_cnt <= 0;
        single_step_started <= 0;
        single_step_quit <= 0;
        abs_read <= 0;
        abs_write <= 0;
        abs_read_data <= 0;
    end

    always @(posedge trickbox_dmi_write.dm_wvalid)begin
      // $display("DMI Write Data = %h \t %h",trickbox_dmi_write.dm_wdata, trickbox_dmi_write.dm_wdata[63:62]);
      if(trickbox_dmi_write.dm_wdata[63:59] === 'b00000)begin 
        $display("[DMI Driver] Push In Command Queue"); 
        command_queue.push_back(rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata));
      end
      else if (trickbox_dmi_write.dm_wdata[63:59] === 'b10000) begin
        //trigger dmi execution
        $display("[DMI Driver] Trigger Command Execution");
        command_trigger = 1;
      end
      else if (trickbox_dmi_write.dm_wdata[63:59] === 'b01000) begin
        $display("[DMI Driver] Delaying the Trigger to Debugger");
        ext_trig_delay = clk_cnt + trickbox_dmi_write.dm_wdata[31:0];
      end
      else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00010) begin
        single_step_ahead_command_queue.push_back(rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
        $display("[DMI Driver] Added this command to the single step (ahead) queue, size=%h",single_step_ahead_command_queue.size());
      end
      else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00100) begin
        single_step_quit_command_queue.push_back(rv_tester_pkg::dmi_req_t'(trickbox_dmi_write.dm_wdata[43:0])); //Mentioning 44 bits to be on safer side
        $display("[DMI Driver] Added this command to the single step (quit) queue, size=%h",single_step_quit_command_queue.size());
      end
      else if (trickbox_dmi_write.dm_wdata[63:59] === 'b00001) begin
        $display("[DMI Driver] Single steping mode enabled, getting count value");
        single_step_instr_cnt = trickbox_dmi_write.dm_wdata[31:0];
      end
    end

    always @(negedge clk) begin
      clk_cnt = clk_cnt + 1;
      if(clk_cnt == ext_trig_delay) begin
       command_trigger = 1;
       $display("[DMI Driver] The delay was executed and asserting the trigger");
      end
    end

    task do_file_writes();
      begin
        file_descr = $fopen("./abs_req_out.txt","w+");
        for (int i=0;i<abs_reg_out_queue.size();i=i+1) begin
          $fdisplay(file_descr, "Addr:%0h Data:%0h",abs_reg_out_queue[i].reg_addr, abs_reg_out_queue[i].reg_data);
        end
        $fclose(file_descr);
      end
    endtask: do_file_writes

    task drive_dmi_cmd(input rv_tester_pkg::dmi_req_t cmd);
      begin
        @(posedge clk)
        dmi_req_valid <= '1;
        dmi_req <= cmd;
        wait (dmi_req_ready == 1);
        @(posedge clk)
        dmi_req_valid <= '0;
        wait (dmi_resp_valid == 1);
        @(posedge clk)
        dmi_resp_ready = 1;
        response_queue.push_back(dmi_resp);
        @(posedge clk)
        dmi_resp_ready = 0;
      end
    endtask: drive_dmi_cmd

    task is_poll_needed(input rv_tester_pkg::dmi_req_t cmd);
      begin
      //decode request type
        if(cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[31] === '1)begin
          $display("[Poll] Seen Halt Req, Doing Poll");
          halt_req = 1;
          poll =1;
        end
        else if(cmd.addr === 'h10 && cmd.op === 'h2 && cmd.data[30] === '1)begin
          $display("[Poll] Seen Resume Req, Doing Poll");
          resume_req = 1;
          poll =1;
        end
        else if(cmd.addr === 'h17 && cmd.op === 'h2)begin
          $display("[Poll] Seen Abstract Command Req, Doing Poll");
          abstr_cmd_req = 1;
          poll =1;
          if(cmd.data[31:24] === 'h0 && cmd.data[17] === 'h1) begin
            //Seen an abstract reg command with rd/write
            $display("[Poll] Seen an abstract command with rd/write");
            if(cmd.data[16] === 'h1)begin 
              $display("[Poll] Seen the abstract command with write");
              abs_write = 1;
            end
            else if(cmd.data[16] === 'h0)begin 
              $display("[Poll] Seen the abstract command with read");
              abs_read = 1;
              abs_data_temp_packet.reg_addr = cmd.data[15:0];
            end
          end
        end
      end
    endtask: is_poll_needed

    task do_polling();
      begin
        $display("[Poll] Starting poll for halt:%h resume:%h abstract:%h",halt_req,resume_req,abstr_cmd_req);
         while(poll)begin 
              //READ DMSTATUS
              @(posedge clk)
              dmi_req_valid <= '1;
              if(halt_req | resume_req)begin
              //  $display("dmstatus haltreq %d resumereq %d\n",halt_req,resume_req);
              dmi_req <= 41'h4500000000;
              end
              else if(abstr_cmd_req)begin
                // $display("abstractcs busy %d\n",abstr_cmd_req);
              dmi_req <= 41'h5900000000;
              end
              else if(abs_read_data)begin
              $display("[Poll] Doing abstract read data for data0");
              dmi_req <= 41'h1100000000;
              end
              wait (dmi_req_ready == 1);
              @(posedge clk)
              dmi_req_valid <= '0;
              wait (dmi_resp_valid == 1);
              @(posedge clk)
              dmi_resp_ready = 1;
                @(posedge clk)
              dmi_resp_ready = 0;
              //check dmstatus
              // $display("\ndmi resp %h\n",dmi_resp.data);
              if(resume_req && dmi_resp.data[17:16] === 2'b11)begin
              resume_req = 0;
              poll = 0;
              $display("[Poll] Clear Resume Req Poll");
              do_file_writes();
              end 
              else if(halt_req && dmi_resp.data[9:8] === 2'b11)begin
              halt_req = 0;
              poll = 0;
              $display("[Poll] Clear Halt Req Poll");
              end
              else if(abstr_cmd_req && dmi_resp.data[12] === 1'b0)begin
              abstr_cmd_req = 0;
              $display("[Poll] Clear Abstract Command Req Poll");
              $display("[Poll] Reading the data output of the abstract command");
              if(abs_read === 'h1)begin
                abs_read_data = 1;
              end
              else begin
                poll = 0;
              end
              end
              else if(abs_read_data) begin
              abs_data_temp_packet.reg_data = dmi_resp.data;
              abs_reg_out_queue.push_back(abs_data_temp_packet);
              abs_read_data = 0;
              poll = 0;
              end
	      end 
      $display("[Poll] Cleared poll for halt:%h resume:%h abstract:%h",halt_req,resume_req,abstr_cmd_req);
      end
    endtask: do_polling

    always @(posedge command_trigger) begin
      $display("[DMI Execution] Starting the execution of Debug Commands");
       if(command_trigger>0) begin
           while(command_queue.size()>0 && single_step_started != 1)begin
            command = command_queue.pop_front();
            $display("[DMI Execution] Popped Cmd ==> addr:%h op:%h data:%h",command.addr,command.op,command.data);
            if(command.op =='h3)begin
              $display("[DMI Execution] Encountered Debug Checkopoint, Switching Control to Assembly");
              break;
            end
            else begin
            drive_dmi_cmd(command);
            is_poll_needed(command);
            do_polling();
            $display("[DMI Execution] Executed the command, commands left out in the queue=%h",command_queue.size());
           end
           end 
            command_trigger = 0;
            $display("[DMI Execution] Clear the Execution Trigger\n");
            @(posedge clk);
       end
    end

endmodule
`endif