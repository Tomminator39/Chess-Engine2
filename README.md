Tom's Chess Engine (2)
---------------------
To-Do List (not in order per se):

- Pawn Structure (Passed pawn, doubled pawns, isolated pawns)
- Improve endgame play
---------------------
Current Features:

Move Generation :
- Bitboards
- Attack Tables
- Opening Book

Search:
- Negamax search with alpha beta pruning
- Transposition Tables
- Iterative Deepening
- Quiescence Search
- SEE + MVV-LVA
- PVS and aspiration windows
- Killer Move, Relative History, and Countermove Heuristics 
- LMR
- NMP
- RFP
- LMP
- FP (Still out on how beneficial this is... maybe with better tuned values)

Evaluation:
- Count Material
- Piece Square Tables
- Tapered Eval
- Safe Mobility
- King safety: Pawn Shield + King Danger score
--------------------
Future Improvements:

Move Generation:
- Magic Bitboards

Search:
- QS and PVS SEE Pruning
- Extensions

Move Ordering:
- Continuation History Heuristcs (CounterMove heuristics is the 1-ply version of this I believe.)
- Capture History
- Improving Heuristics
- IIR

Evaluation:
- Pawn Structure improvements
- King Safety improvements
- Texel Tuning values
---------------------
Version Descriptions:

TCE_v1: bitboards, attack tables, negamax with alphabeta, iterative deepening, quiescence search, transposition tables, mvv-lva, piece square tables, tapered eval, opening book. pvs, aspiration windows.
TCE_v2: killer move, relative history, and countermove heuristics
TCE_v3: LMR
TCE_v4: NMP
TCE_v5: (safe) Mobility + Bishop pair bonus
TCE_v6: RFP + LMP
TCE_v7: FP
TCE_v8: Pawn Shield + King Danger Score

---------------------
Fastchess test command:

fastchess -engine cmd="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\build\ChessEngine.exe" name="TCE_Current" -engine cmd="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\snapshots\TCE_v7.exe" name="TCE_v7" -openings file="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\books\8moves_v3.pgn" format=pgn order=random -each tc=10+1.0 proto=uci -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -rounds 5000 -repeat -concurrency 6 -recover -draw movenumber=30 movecount=6 score=15 -resign movecount=3 score=500

fastchess -engine cmd="./ChessEngine" dir="/home/tomh/Documents/GitHub/Chess-Engine2/build" name="TCE_Current" -engine cmd="./TCE_v7" dir="/home/tomh/Documents/GitHub/Chess-Engine2/snapshots" name="TCE_v7" -openings file="/home/tomh/Documents/GitHub/Chess-Engine2/books/8moves_v3.pgn" format=pgn order=random -each tc=10+1.0 proto=uci -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -rounds 5000 -repeat -concurrency 6 -recover -draw movenumber=30 movecount=6 score=15 -resign movecount=3 score=500