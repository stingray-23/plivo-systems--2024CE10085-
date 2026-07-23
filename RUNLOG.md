# Run Log
* Experiment 1: profile A, delay_ms=40. Result: INVALID. Miss: 4.73%. Overhead: 1.54x. Change: Delay was too tight for block-XOR FEC to recover frame 0 using frame 1.
* Experiment 2: profile A, delay_ms=60. Result: VALID. Miss: 0.27%. Overhead: 1.54x. Change: Increased jitter buffer wait time to allow block completion.
* Experiment 3: profile B, delay_ms=60. Result: INVALID. Miss: 27.87%. Change: Profile B has much higher jitter and packet drops; 60ms is too tight for successful recovery.
* Experiment 4: profile B, delay_ms=100. Result: VALID. Overhead: 1.54x. Change: Increased playout delay to accommodate Profile B's hostile network conditions. Final safe score locked in.