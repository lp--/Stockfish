/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "misc.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

using namespace std;

namespace {

const vector<string> Defaults = {
"8/3rbk1p/4bpp1/2p5/P2n1B2/2NB2P1/4PK1P/1R6 b - - 2 33",
"8/3rbk1p/4bpp1/8/P1pnBB2/2N3P1/4PK1P/1R6 b - - 1 34",
"8/3rbk1p/4bpp1/8/P1p1B3/1nN1B1P1/4PK1P/1R6 b - - 3 35",
"8/3r1k1p/4bpp1/1N6/Pbp1B3/1n2B1P1/4PK1P/1R6 b - - 5 36",
"8/3r1k1p/4bpp1/bN6/P1p5/1n2B1P1/2B1PK1P/1R6 b - - 7 37",
"8/3r1k1p/4bpp1/bN6/P1p5/4B1P1/2BnPK1P/2R5 b - - 9 38",
"8/3r1k1p/4bpp1/1N6/Pbp5/4B1PP/2BnPK2/2R5 b - - 0 39",
"8/3r2kp/4bpp1/1N6/Pbp3P1/4B2P/2BnPK2/2R5 b - - 0 40",
"8/3r1bkp/5pp1/1N6/Pbp3P1/4B2P/2BnPK2/R7 b - - 2 41",
"8/3r1bk1/5ppp/1N6/Pbp3P1/4B2P/2BnPK2/2R5 b - - 1 42",

"2rq1rk1/1p3ppp/p2p4/3Np1b1/4P1b1/1nPQ1N1P/PP3PP1/3R1RK1 w - - 0 16",
"2rq1rk1/1p3ppp/p2pb3/3Np1b1/4P3/1PPQ1N1P/1P3PP1/3R1RK1 w - - 1 17",
"2r2rk1/1p3ppp/p2pb3/3Np1q1/4P3/1PPQ3P/1P3PP1/3R1RK1 w - - 0 18",
"5rk1/1p3ppp/p1rpb3/3Np1q1/2P1P3/1P1Q3P/1P3PP1/3R1RK1 w - - 1 19",
"5rk1/1p3ppp/p1rpb3/3Np3/2P1P3/1P4qP/1P3PP1/3R1RK1 w - - 0 20",
"5r1k/1p2Nppp/p1rpb3/4p3/2P1P3/1P4qP/1P3PP1/3R1RK1 w - - 2 21",
"5r1k/1p2Nppp/pr1pb3/4p3/2P1P3/1P4PP/1P4P1/3R1RK1 w - - 1 22",
"5r1k/1p2Np1p/pr1pb1p1/4p3/2P1P3/1P3RPP/1P4P1/3R2K1 w - - 0 23",
"5r2/1p2Npkp/pr1pb1p1/4p3/2P1P1P1/1P3R1P/1P4P1/3R2K1 w - - 1 24",
"5r2/1p3pkp/p1rpb1p1/3Np3/2P1P1P1/1P3R1P/1P4P1/3R2K1 w - - 3 25",

"rnbqk1nr/ppp1ppbp/6p1/3p4/8/3P2P1/PPP1PPBP/RNBQK1NR w KQkq d6 0 3",
"rnbqk1nr/ppp1ppbp/6p1/8/2p5/3P2P1/PP2PPBP/RNBQK1NR w KQkq - 0 4",
"rnbqk1nr/pp2ppbp/2p3p1/8/Q1p5/3P2P1/PP2PPBP/RNB1K1NR w KQkq - 0 5",
"rnbqk2r/pp2ppbp/2p2np1/8/Q1P5/6P1/PP2PPBP/RNB1K1NR w KQkq - 1 6",
"rnbq1rk1/pp2ppbp/2p2np1/8/Q1P5/5NP1/PP2PPBP/RNB1K2R w KQ - 4 7",
"r1bq1rk1/pp1nppbp/2p2np1/8/Q1P5/5NP1/PP2PPBP/RNB2RK1 w - - 7 8",
"r1bq1rk1/pp1nppbp/2p3p1/8/2P1n3/Q4NP1/PP2PPBP/RNB2RK1 w - - 9 9",
"r1bq1rk1/pp2ppbp/1np3p1/8/2P1n3/Q3BNP1/PP2PPBP/RN3RK1 w - - 11 10",
"r1bq1rk1/pp2ppbp/1np3p1/8/2P5/Q3BNP1/PP1nPPBP/R4RK1 w - - 0 11",
"r2q1rk1/pp2ppbp/1np3p1/8/2P3b1/Q3B1P1/PP1NPPBP/R4RK1 w - - 1 12",

"2nr1rk1/p2qppbp/1pp3p1/P7/1QP1N1b1/4B1P1/1P2PPBP/R3R1K1 w - - 0 17 ",
"2nr1rk1/3qppbp/1pp3p1/8/1QP1N1b1/4B1P1/1P2PPBP/R3R1K1 w - - 0 18",
"2nr1rk1/3qppbp/2p3p1/1pP5/1Q2N1b1/4B1P1/1P2PPBP/R3R1K1 w - - 0 19",
"2nr1rk1/3qppb1/2p3pp/1pP3N1/1Q4b1/4B1P1/1P2PPBP/R3R1K1 w - - 0 20",
"2nr1rk1/3qppb1/2p3pp/1pP5/1Q2N3/4B1Pb/1P2PPBP/R3R1K1 w - - 2 21",
"2nr1rk1/4ppb1/2p3pp/1pP5/1Q2N3/4B1Pq/1P2PP1P/R3R1K1 w - - 0 22",
"2n2rk1/4ppb1/R1p3pp/1pPr4/1Q2N3/4B1Pq/1P2PP1P/4R1K1 w - - 2 23",
"2n2rk1/3qppb1/R1p3pp/1pPr4/1Q6/4B1P1/1P1NPP1P/4R1K1 w - - 4 24",
"2n2rk1/4ppb1/R1p3pp/1pPr4/1Q2N3/4B1Pq/1P2PP1P/4R1K1 w - - 6 25",
"2n2rk1/3qppb1/R1p3pp/1pPr4/1Q6/4B1P1/1P1NPP1P/4R1K1 w - - 8 26",

"r1bq1rk1/pp2p2p/6p1/3P1p2/1n1NP3/8/P2QBPPP/R3K2R b KQ - 1 14",
"r1bq1rk1/1p2p2p/6p1/p2P1p2/1n1NP3/P7/3QBPPP/R3K2R b KQ - 0 15",
"r1bq1rk1/1p2p2p/n5p1/p2P1p2/2BNP3/P7/3Q1PPP/R3K2R b KQ - 2 16",
"r1bq1rk1/1p2p2p/n5p1/p2P4/2BNp3/P7/3Q1PPP/R4RK1 b - - 2 17",
"r1bq1rk1/1p2p2p/6p1/p1nP4/2BNp3/P3Q3/5PPP/R4RK1 b - - 4 18",
"r1bq1rk1/4p2p/1pN3p1/p1nP4/2B1p3/P3Q3/5PPP/R4RK1 b - - 1 19",
"r1b2rk1/4p2p/1pNq2p1/p1nP4/2B1p3/P3Q3/5PPP/1R3RK1 b - - 3 20",
"r1b2rk1/7p/1RNqp1p1/p1nP4/2B1p3/P3Q3/5PPP/5RK1 b - - 0 21",
"r1b2rk1/3n3p/2Nqp1p1/p2P4/2B1p3/P3Q3/5PPP/1R3RK1 b - - 2 22",
"r1b2rk1/7p/2NqPnp1/p7/2B1p3/P3Q3/5PPP/1R3RK1 b - - 0 23",

"3RQnk1/p1q2rpp/2p5/1p2p3/4P3/PP5P/2P1N1P1/6K1 b - - 12 35",
"3RQnk1/p1q2r1p/2p3p1/1p2p3/4P3/PP5P/2P1N1PK/8 b - - 1 36",
"3RQnk1/p4r1p/1qp3p1/1p2p3/4P3/PP4NP/2P3PK/8 b - - 3 37",
"3RQnk1/p4r1p/2p3p1/1pq1p3/2P1P3/PP4NP/6PK/8 b - c3 0 38",
"3RQnk1/p4r1p/2p3p1/2q1p3/2P1P3/P5NP/6PK/8 b - - 0 39",
"2R1Qnk1/p6p/2p3p1/2q1p3/2P1P3/P5NP/5rPK/8 b - - 2 40",
"2R2nk1/p6p/2Q3p1/4p3/2q1P3/P5NP/5rPK/8 b - - 0 41",
"5nk1/p6p/2R3p1/4p3/4P3/P5NP/5rPK/8 b - - 0 42",
"5n2/p5kp/R5p1/4p3/4P3/P5NP/5rPK/8 b - - 2 43",
"5n2/p5k1/R5p1/4p2p/4P3/P5NP/5rP1/6K1 b - - 1 44"
};

} // namespace

/// benchmark() runs a simple benchmark by letting Stockfish analyze a set
/// of positions for a given limit each. There are five parameters: the
/// transposition table size, the number of search threads that should
/// be used, the limit value spent for each position (optional, default is
/// depth 13), an optional file name where to look for positions in FEN
/// format (defaults are the positions defined above) and the type of the
/// limit value: depth (default), time in millisecs or number of nodes.

void benchmark(const Position& current, istream& is) {

  string token;
  Search::LimitsType limits;
  vector<string> fens;

  // Assign default values to missing arguments
  string ttSize    = (is >> token) ? token : "16";
  string threads   = (is >> token) ? token : "1";
  string limit     = (is >> token) ? token : "13";
  string fenFile   = (is >> token) ? token : "default";
  string limitType = (is >> token) ? token : "depth";

  Options["Hash"]    = ttSize;
  Options["Threads"] = threads;
  Search::reset();

  if (limitType == "time")
      limits.movetime = stoi(limit); // movetime is in ms

  else if (limitType == "nodes")
      limits.nodes = stoi(limit);

  else if (limitType == "mate")
      limits.mate = stoi(limit);

  else
      limits.depth = stoi(limit);

  if (fenFile == "default")
      fens = Defaults;

  else if (fenFile == "current")
      fens.push_back(current.fen());

  else
  {
      string fen;
      ifstream file(fenFile);

      if (!file.is_open())
      {
          cerr << "Unable to open file " << fenFile << endl;
          return;
      }

      while (getline(file, fen))
          if (!fen.empty())
              fens.push_back(fen);

      file.close();
  }

  uint64_t nodes = 0;
  Search::StateStackPtr st;
  TimePoint elapsed = now();

  for (size_t i = 0; i < fens.size(); ++i)
  {
      Position pos(fens[i], Options["UCI_Chess960"], Threads.main());

      cerr << "\nPosition: " << i + 1 << '/' << fens.size() << endl;

      if (limitType == "perft")
          nodes += Search::perft<true>(pos, limits.depth * ONE_PLY);

      else
      {
          Threads.start_thinking(pos, limits, st);
          Threads.main()->join();
          nodes += Search::RootPos.nodes_searched();
      }
  }

  elapsed = now() - elapsed + 1; // Ensure positivity to avoid a 'divide by zero'

  dbg_print(); // Just before to exit

  cerr << "\n==========================="
       << "\nTotal time (ms) : " << elapsed
       << "\nNodes searched  : " << nodes
       << "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
}
