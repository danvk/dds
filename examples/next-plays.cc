/**
 * Usage:
 *   ./next-plays DEAL_PBN <C|D|H|S|N> <N|S|E|W> play1 play2 ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/dll.h"

const char* USAGE = "./next-plays DEAL_PBN <C|D|H|S|N> <N|S|E|W> play1 play2 ...";


#define SPADES   0
#define HEARTS   1
#define DIAMONDS 2
#define CLUBS    3
#define NOTRUMP  4

#define NORTH    0
#define EAST     1
#define SOUTH    2
#define WEST     3

int NEXT_PLAYER[] = {EAST, SOUTH, WEST, NORTH};

void bad_trump(const char* trump) {
  fprintf(stderr, "Invalid trump: '%s' (expected N, S, H, D, C)\n", trump);
  exit(1);
}

int parse_trump(const char* trump) {
  if (strlen(trump) != 1) {
    bad_trump(trump);
  }
  char c = toupper(trump[0]);
  if (c == 'N') return NOTRUMP;
  if (c == 'S') return SPADES;
  if (c == 'H') return HEARTS;
  if (c == 'D') return DIAMONDS;
  if (c == 'C') return CLUBS;

  bad_trump(trump);
  return 0;  // unreachable
}

void bad_player(const char* player) {
  fprintf(stderr, "Invalid player: '%s' (expected N, S, E, W)\n", player);
  exit(1);
}

int parse_player(const char* player) {
  if (strlen(player) != 1) {
    bad_player(player);
  }
  char c = toupper(player[0]);
  if (c == 'N') return NORTH;
  if (c == 'S') return SOUTH;
  if (c == 'E') return EAST;
  if (c == 'W') return WEST;

  bad_player(player);
  return 0;  // unreachable
}


unsigned short int dbitMapRank[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};

unsigned char dcardRank[16] =
{ 
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char dcardSuit[5] = { 'S', 'H', 'D', 'C', 'N' };
unsigned char dcardHand[4] = { 'N', 'E', 'S', 'W' };

void equals_to_string(int equals, char * res) {
  int p = 0;
  int m = equals >> 2;
  for (int i = 15; i >= 2; i--)
  {
    if (m & static_cast<int>(dbitMapRank[i]))
      res[p++] = static_cast<char>(dcardRank[i]);
  }
  res[p] = 0;
}

void PrintFut(const futureTricks& fut) {
  printf("%6s %-6s %-6s %-6s %-6s\n",
         "card", "suit", "rank", "equals", "score");

  for (int i = 0; i < fut.cards; i++) {
    char res[15] = "";
    equals_to_string(fut.equals[i], res);
    printf("%6d %-6c %-6c %-6s %-6d\n",
           i,
           dcardSuit[ fut.suit[i] ],
           dcardRank[ fut.rank[i] ],
           res,
           fut.score[i]);
  }
  printf("\n");
}



int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "Not enough arguments. Usage:\n\n  %s\n", USAGE);
    return 1;
  }

  char *deal_pbn = argv[1],
       *suit_str = argv[2],
       *declarer_str = argv[3];

  int declarer = parse_player(declarer_str);

  dealPBN dlPBN;

  dlPBN.trump = parse_trump(suit_str);
  dlPBN.first = NEXT_PLAYER[declarer];

  strcpy(dlPBN.remainCards, deal_pbn);

  // previously-played cards on this trick
  dlPBN.currentTrickSuit[0] = 0;
  dlPBN.currentTrickRank[0] = 0;
  dlPBN.currentTrickSuit[1] = 0;
  dlPBN.currentTrickRank[1] = 0;
  dlPBN.currentTrickSuit[2] = 0;
  dlPBN.currentTrickRank[2] = 0;

  int target = -1;
  int solutions = 3;
  int mode = 0;

  SetMaxThreads(0);

  futureTricks fut;
  int res = SolveBoardPBN(dlPBN, target, solutions, mode, &fut, 0);
  if (res != RETURN_NO_FAULT) {
    char error[80];
    ErrorMessage(res, error);
    fprintf(stderr, "SolveBoard failed: %d %s\n", res, error);
    return 1;
  }

  // PrintPBNHand

  PrintFut(fut);

  return 0;
}
