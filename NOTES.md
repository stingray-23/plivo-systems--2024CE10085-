Implemented a Forward Error Correction (FEC) protocol using Block XOR. To circumvent the high RTT penalty of ARQ feedback loops, the sender groups frames into pairs (Frame 0 and Frame 1) and transmits a 3rd recovery packet containing the XOR of both payloads. This achieves immediate drop recovery with a static bandwidth overhead of 1.54x, staying strictly under the 2.00x limit. The receiver uses a pre-allocated array as a jitter buffer, executing XOR recovery if a primary frame is missing, and synchronizes playout strictly with the harness deadline schedule using non-blocking timeouts.

Target delay graded at: 100ms.
What breaks it: Burst network drops where both a primary frame and its corresponding XOR recovery packet are lost within the same 2-frame block, or extreme jitter exceeding the 100ms buffer window limit.

— Shashank Kumar