/**
 * Usage:
 *   ./next-plays DEAL_PBN <C|D|H|S|N> <N|S|E|W> play1 play2 ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include "../include/dll.h"
#include "../src/PBN.h"

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

struct Play {
  int suit;  // SPADES, HEARTS, DIAMONDS, CLUBS
  int rank;  // 2-14
};

int NEXT_PLAYER[] = {EAST, SOUTH, WEST, NORTH};

std::string StringPrintf(const char* format, ...) {
  int buf_size = 256;
  char* buffer = static_cast<char*>(malloc(static_cast<size_t>(buf_size)));
  va_list args;
  va_start(args, format);
  int ret = vsnprintf(buffer, static_cast<size_t>(buf_size), format, args);
  va_end(args);
  if (ret < 0) {
    return "";
  } else if (ret >= buf_size) {
    free(buffer);
    buf_size = ret + 1;
    printf("buf_size: %d\n", buf_size);
    buffer = static_cast<char*>(malloc(static_cast<size_t>(buf_size)));
    va_start(args, format);
    ret = vsnprintf(buffer, static_cast<size_t>(buf_size), format, args);
    va_end(args);
    if (ret >= buf_size) {
      return "";
    }
  }

  std::string out(buffer);
  free(buffer);
  return out;
}

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

void bad_play(const char* play) {
  fprintf(stderr, "Invalid play: '%s' (expected, e.g. 2D, 9C, TH, AS)\n", play);
  exit(1);
}

Play parse_play(const char* play) {
  if (strlen(play) != 2) {
    bad_play(play);
  }

  Play p;

  switch (toupper(play[0])) {
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      p.rank = play[0] - '0';
      break;
    case 'T': p.rank = 10; break;
    case 'J': p.rank = 11; break;
    case 'Q': p.rank = 12; break;
    case 'K': p.rank = 13; break;
    case 'A': p.rank = 14; break;
    default:
      bad_play(play);
  }

  switch (toupper(play[1])) {
    case 'H': p.suit = HEARTS; break;
    case 'S': p.suit = SPADES; break;
    case 'D': p.suit = DIAMONDS; break;
    case 'C': p.suit = CLUBS; break;
    default:
      bad_play(play);
  }
  return p;
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

std::string OneTrickToJSON(char suit, char rank, int score) {
  char buf[100];
  sprintf(buf, "{\"suit\": \"%c\", \"rank\": \"%c\", \"score\": %d}",
          suit, rank, score);
  return std::string(buf);
}

std::string FutureTricksToJSON(const futureTricks& fut, int player, int ns_tricks, int ew_tricks) {
  int n = 0;
  std::string player_str(1, dcardHand[player]);
  std::string out = "{\"player\": \"" + player_str + "\","
                     "\"tricks\": {"
                      "\"ns\": " + std::to_string(ns_tricks) +
                    ", \"ew\": " + std::to_string(ew_tricks) +
                    "}, \"plays\":[";
  for (int i = 0; i < fut.cards; i++) {
    if (i) out += ",";
    out += OneTrickToJSON(
           dcardSuit[ fut.suit[i] ],
           dcardRank[ fut.rank[i] ],
           fut.score[i]);

    char res[15] = "";
    equals_to_string(fut.equals[i], res);
    int len = static_cast<int>(strlen(res));
    for (int j = 0; j < len; j++) {
      char rank = res[j];
      out += ",";
      out += OneTrickToJSON(
             dcardSuit[ fut.suit[i] ],
             rank,
             fut.score[i]);
    }
  }
  out += "]}";
  return out;
}

std::string card_to_str(const Play& card) {
  std::string out = "  ";
  char c;
  if (card.rank < 10) {
    c = '0' + card.rank;
  } else if (card.rank == 10) {
    c = 'T';
  } else if (card.rank == 11) {
    c = 'J';
  } else if (card.rank == 12) {
    c = 'Q';
  } else if (card.rank == 13) {
    c = 'K';
  } else if (card.rank == 14) {
    c = 'A';
  } else {
    c = '?';
  }
  out[0] = c;
  out[1] = dcardSuit[card.suit];

  return out;
}

// Determine which player won the current trick.
// This clears out the current trick, updates dl->first and returns dl->first.
int determine_winner(deal* dl, int last_suit, int last_rank) {
  if (dl->currentTrickRank[0] == 0 ||
      dl->currentTrickRank[1] == 0 ||
      dl->currentTrickRank[2] == 0) {
    fprintf(stderr, "Tried to find winner of incomplete trick.\n");
    exit(1);
  }

  Play plays[] = {
    {dl->currentTrickSuit[0], dl->currentTrickRank[0]},
    {dl->currentTrickSuit[1], dl->currentTrickRank[1]},
    {dl->currentTrickSuit[2], dl->currentTrickRank[2]},
    {last_suit, last_rank}
  };

  int led_suit = plays[0].suit;
  int top_suit = led_suit;
  int top_rank = plays[0].rank;
  int winner = dl->first;
  int player = dl->first;
  for (int i = 1; i < 4; i++) {
    player = NEXT_PLAYER[player];
    int suit = plays[i].suit,
        rank = plays[i].rank;
    if (suit == top_suit && rank > top_rank) {
      top_rank = rank;
      winner = player;
    } else if (suit == dl->trump && top_suit != dl->trump) {
      top_rank = rank;
      top_suit = suit;
      winner = player;
    }
  }

  dl->currentTrickSuit[0] = 0;
  dl->currentTrickRank[0] = 0;
  dl->currentTrickSuit[1] = 0;
  dl->currentTrickRank[1] = 0;
  dl->currentTrickSuit[2] = 0;
  dl->currentTrickRank[2] = 0;
  dl->first = winner;
  return winner;
}

extern "C" {

char* solve(char* deal_pbn, char* suit_str, char* declarer_str, int num_plays, Play* plays) {
  char* out = (char*)malloc(1024);
  int declarer = parse_player(declarer_str);
  int player = NEXT_PLAYER[declarer];

  deal dl;

  dl.trump = parse_trump(suit_str);
  dl.first = player;

  int res = ConvertFromPBN(deal_pbn, dl.remainCards);
  if (res != RETURN_NO_FAULT) {
    char error[80];
    ErrorMessage(res, error);
    fprintf(stderr, "ConvertFromPBN failed: %d %s\n", res, error);
    strcpy(out, "{\"error\": 1}");
    return out;
  }

  // previously-played cards on this trick
  dl.currentTrickSuit[0] = 0;
  dl.currentTrickRank[0] = 0;
  dl.currentTrickSuit[1] = 0;
  dl.currentTrickRank[1] = 0;
  dl.currentTrickSuit[2] = 0;
  dl.currentTrickRank[2] = 0;

  int ew_tricks = 0,
      ns_tricks = 0;
  for (int i = 0; i < num_plays; i++) {
    const Play& p = plays[i];
    unsigned int cards_in_suit = dl.remainCards[player][p.suit];
    unsigned int card = 1 << p.rank;
    if ((cards_in_suit & card) == 0) {
      fprintf(stderr, "Player %d cannot play %s\n", player, card_to_str(p).c_str());
      exit(1);
    }
    dl.remainCards[player][p.suit] -= card;

    if (i % 4 == 3) {
      player = determine_winner(&dl, p.suit, p.rank);
      if (player == NORTH || player == SOUTH) {
        ns_tricks++;
      } else {
        ew_tricks++;
      }
    } else {
      dl.currentTrickSuit[i % 4] = p.suit;
      dl.currentTrickRank[i % 4] = p.rank;
      player = NEXT_PLAYER[player];
    }
  }

  int target = -1;
  int solutions = 3;
  int mode = 2;

  SetMaxThreads(0);

  futureTricks fut;
  res = SolveBoard(dl, target, solutions, mode, &fut, 0);
  if (res != RETURN_NO_FAULT) {
    char error[80];
    ErrorMessage(res, error);
    fprintf(stderr, "SolveBoard failed: %d %s\n", res, error);
    strcpy(out, "{\"error\": 1}");
    return out;
  }

  std::string json = FutureTricksToJSON(fut, player, ns_tricks, ew_tricks);
  strcpy(out, json.c_str());
  return out;
}

}
