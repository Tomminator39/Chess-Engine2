Tom's Chess Engine (2)
---------------------
To-Do List:

- improve move ordering with see and heuristics
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
- MVV-LVA
- PVS and aspiration windows

Evaluation:
- Count Material
- Piece Square Tables
- Tapered Eval
--------------------
Future Improvements:

Move Generation:
- Magic Bitboards

Search:
- Killer Move Heuristic
- Countermove Heuristic
- Butterfly History Heuristic
- Aspiration Windows
- Principal Variation Search -> Read this: https://www.chessprogramming.org/PVS_and_Aspiration
- idk man just follow improved connorpasta

Evaluation:
- PSQ optimized with texel tuning
- King Safety
- Pawn Structure (and pawns in general)
---------------------
Version Descriptions:

TCE_v1: bitboards, attack tables, negamax with alphabeta, iterative deepening, quiescence search, transposition tables, mvv-lva, piece square tables, tapered eval, opening book. pvs, aspiration windows.
TCE_v2: 

---------------------
Fastchess test command:

fastchess -engine cmd="path" name="TCE_Current" -engine cmd="path" name="TCE_v1" -openings file="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\books\8moves_v3.pgn" format=pgn order=random -each tc=10+1.0 proto=uci -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -rounds 5000 -repeat -concurrency 4 -recover -draw movenumber=30 movecount=6 score=15 -resign movecount=3 score=500

fastchess -engine cmd="path" name="TCE_Current" -engine cmd="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\snapshots\Pre-PVS.exe" name="Pre-PVS" -openings file="C:\Users\Tomhi\Documents\GitHub\Chess-Engine2\books\8moves_v3.pgn" format=pgn order=random -each tc=10+1.0 proto=uci -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -rounds 5000 -repeat -concurrency 4 -recover -draw movenumber=30 movecount=6 score=15 -resign movecount=3 score=500