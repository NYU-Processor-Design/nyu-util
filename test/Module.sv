module Module (
    input clk,
    input nReset,

    input in,

    output logic out
);

  logic sync;

  always_ff @(posedge clk, negedge nReset) begin
    if (!nReset) begin
      sync <= 0;
      out  <= 0;
    end else begin
      sync <= in;
      out  <= sync;
    end
  end

endmodule
