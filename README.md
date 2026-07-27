Tom's Chess Engine (2)
---------------------
To-Do List:

- LMR
---------------------
Current Features:

Move Generation :
- Bitboards
- Attack Tables

Search:
- Negamax search with alpha beta pruning
- Transposition Tables
- Iterative Deepening
- Quiescence Search
- SEE + MVV-LVA
- PVS and aspiration windows
- Killer Move, Relative History, and Countermove Heuristics 

Evaluation:
- Count Material
- Piece Square Tables
- Tapered Eval
--------------------
Future Improvements:

Move Generation:
- Magic Bitboards

Search: (Just follow connorpasta?)
- Late Move Reductions
- Null Move Pruning
- RFP
- Extensions
- SEE pruning in main search (also need early exit SEE for this!)

Move Ordering:
- Continuation History Heuristcs (CounterMove heuristics is the 1-ply version of this I believe.)
- Capture History

Evaluation:
- PSQ optimized with texel tuning
- King Safety
- Pawn Structure (and pawns in general)
---------------------
Version Descriptions:

TCE_v1: bitboards, attack tables, negamax with alphabeta, iterative deepening, quiescence search, transposition tables, mvv-lva, piece square tables, tapered eval, opening book. pvs, aspiration windows.
TCE_v2: killer move, relative history, and countermove heuristics

---------------------
Fastchess test command:

fastchess -engine cmd="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\build\ChessEngine.exe" name="TCE_Current" -engine cmd="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\snapshots\TCE_v1.exe" name="TCE_v1" -openings file="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\books\8moves_v3.pgn" format=pgn order=random -each tc=10+1.0 proto=uci -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -rounds 5000 -repeat -concurrency 6 -recover -draw movenumber=30 movecount=6 score=15 -resign movecount=3 score=500