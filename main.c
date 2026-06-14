#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


// Global defies, enums, structs, vars:

enum pieces_enum {empty=0,wp=2,wb=3,wn=4,wr=5,wq=6,wk=7,bp=10,bb=11,bn=12,br=13,bq=14,bk=15};
enum turns {white=1, black=-1};

const char pieces_str[][3] = {"  ","??","wp","wb","wn","wr","wq","wk","??","??","bp","bb","bn","br","bq","bk"};

struct move {
  int start;
  int end;
};

struct board {
  char piece[64];
  char state; // [5] {BK_MOVED,LBR_MOVED,RBR_MOVED,WK_MOVED,LWR_MOVED,RWR_MOVED} [0]
  struct move last_move;
};

const int pawn_value_table[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
   50, 50, 50, 50, 50, 50, 50, 50,
   10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 27, 27, 10,  5,  5,
    0,  0,  0, 25, 25,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-25,-25, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_value_table[] = {
  -50,-40,-30,-30,-30,-30,-40,-50,
  -40,-20,  0,  0,  0,  0,-20,-40,
  -30,  0, 10, 15, 15, 10,  0,-30,
  -30,  5, 15, 20, 20, 15,  5,-30,
  -30,  0, 15, 20, 20, 15,  0,-30,
  -30,  5, 10, 15, 15, 10,  5,-30,
  -40,-20,  0,  5,  5,  0,-20,-40,
  -50,-40,-20,-30,-30,-20,-40,-50,
};

const int bishop_value_table[] = {
  -20,-10,-10,-10,-10,-10,-10,-20,
  -10,  0,  0,  0,  0,  0,  0,-10,
  -10,  0,  5, 10, 10,  5,  0,-10,
  -10,  5,  5, 10, 10,  5,  5,-10,
  -10,  0, 10, 10, 10, 10,  0,-10,
  -10, 10, 10, 10, 10, 10, 10,-10,
  -10,  5,  0,  0,  0,  0,  5,-10,
  -20,-10,-40,-10,-10,-40,-10,-20,
};

const int king_middlegame_value[] = {
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -20, -30, -30, -40, -40, -30, -30, -20,
  -10, -20, -20, -20, -20, -20, -20, -10, 
   20,  20,   0,   0,   0,   0,  20,  20,
   20,  30,  10,   0,   0,  10,  30,  20
};

const int king_endgame_value[] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};


// Function declaration for recursion:

int eval(const struct board* const my_board, const struct move* const my_move, const int turn, const int depth, const int max_seq_depth);

int engine_move(const struct board* const my_board, struct move* const next_move, const int turn, const int depth, const int max_seq_depth);


// Functions:

void init_board(struct board* const my_board) {
  const char default_row[] = {wr, wn, wb, wq, wk, wb, wn, wr};
  for (int i = 0; i < 64; i++) {
    my_board->piece[i] = empty;
  }
  for (int x = 0; x < 8; x++) {
    my_board->piece[x] = default_row[x]+8;
    my_board->piece[x+7*8] = default_row[x];
    my_board->piece[x+8] = bp;
    my_board->piece[x+6*8] = wp;
  }
  my_board->state = 0;
}

const char* lookup_piece(const char piece) {
  return pieces_str[piece];
}

bool is_white(const char piece) {
  return piece < 8;
}

void draw_board(const struct board* const my_board) {
  printf("\e[0;40m");
  printf("  ");
  for (char c = 'A'; c <= 'H'; c++) {
    printf("%c ", c);
  }
  printf("\n");
  for (int y = 0; y < 8; y++) {
    printf("%d ", 8-y);
    for (int x = 0; x < 8; x++) {
      if ((x+y)%2 == 0) {
        printf("\e[1;47m");
      } else {
        printf("\e[1;43m");
      }
      if (is_white(my_board->piece[y*8+x])) {
        printf("\e[97m");
      } else {
        printf("\e[30m");
      }
      printf("%c ", lookup_piece(my_board->piece[y*8+x])[1]);
    }
    printf("\e[0;40m");
    printf(" %d\n", 8-y);
  }
  printf("  ");
  for (char c = 'A'; c <= 'H'; c++) {
    printf("%c ", c);
  }
  printf("\n\n");
}

int change_notation_to_int(const char* const move_str) {
  return 8*(8-(move_str[1]-'0')) + (move_str[0]-'a');
}

void change_notation_to_str(const int move_int, char* move_str) {
  move_str[0] = 'a'+(move_int%8);
  move_str[1] = '0'+(8-move_int/8);
  move_str[2] = 0;
}

bool can_do_white_en_passant(const struct board* const my_board, const struct move* const my_move) {
  if (my_move->start/8 != 3 || my_move->end/8 != 2) {
    return false;
  }
  if (abs(my_move->end%8 - my_move->start%8) != 1) {
    return false;
  }
  if (my_board->piece[my_move->end+8] != bp) {
    return false;
  }
  if (my_board->last_move.end - my_board->last_move.start != 16) {
    return false;
  }
  if (my_board->last_move.end != my_move->end+8) {
    return false;
  }
  return true;
}

bool can_do_black_en_passant(const struct board* const my_board, const struct move* const my_move) {
  if (my_move->start/8 != 4 || my_move->end/8 != 5) {
    return false;
  }
  if (abs(my_move->end%8 - my_move->start%8) != 1) {
    return false;
  }
  if (my_board->piece[my_move->end-8] != wp) {
    return false;
  }
  if (my_board->last_move.end - my_board->last_move.start != -16) {
    return false;
  }
  if (my_board->last_move.end != my_move->end-8) {
    return false;
  }
  return true;
}

bool is_legal_pawn(const struct board* const my_board, const struct move* const my_move) {
  if (is_white(my_board->piece[my_move->start])) {
    if (my_move->start-my_move->end == 8 && my_board->piece[my_move->end] == empty) {
      return true;
    }
    if (my_move->start-my_move->end == 16 && my_move->start / 8 == 6 && 
        my_board->piece[my_move->end] == empty && my_board->piece[my_move->end+8] == empty) {
      return true;
    }
    if (my_board->piece[my_move->end] != empty && !is_white(my_board->piece[my_move->end]) &&
        (my_move->start-my_move->end == 7 || my_move->start-my_move->end == 9)) {
      return true;
    }
    if (can_do_white_en_passant(my_board, my_move)) {
      return true;
    }
  } else {
    if (my_move->start-my_move->end == -8 && my_board->piece[my_move->end] == empty) {
      return true;
    }
    if (my_move->start-my_move->end == -16 && my_move->start / 8 == 1 && 
        my_board->piece[my_move->end] == empty && my_board->piece[my_move->end-8] == empty) {
      return true;
    }
    if (my_board->piece[my_move->end] != empty && is_white(my_board->piece[my_move->end]) &&
        (my_move->start-my_move->end == -7 || my_move->start-my_move->end == -9)) {
      return true;
    }
    if (can_do_black_en_passant(my_board, my_move)) {
      return true;
    }
  }
  return false;
}

bool is_legal_knight(const struct board* const my_board, const struct move* const my_move) {
  int d_move_y = my_move->end/8-my_move->start/8;
  int d_move_x = my_move->end%8-my_move->start%8;

  if ((abs(d_move_y) == 2 && abs(d_move_x) == 1)) {
    return true;
  }

  if ((abs(d_move_y) == 1 && abs(d_move_x) == 2)) {
    return true;
  }

  return false;
}

bool is_legal_bishop(const struct board* const my_board, const struct move* const my_move) {
  int d_move_y = my_move->end/8 - my_move->start/8;
  int d_move_x = my_move->end%8 - my_move->start%8;

  if (abs(d_move_y) != abs(d_move_x)) {
    // printf("Wrong angle!\n");
    return false;
  }

  int dx = (d_move_x > 0) ? 1 : -1;
  int dy = (d_move_y > 0) ? 8 : -8;

  
  for (int pos = my_move->start+dx+dy; pos != my_move->end; pos += dx+dy) {
    if (my_board->piece[pos] != empty) {
      return false;
    }
  }

  return true;
}

bool is_legal_rook(const struct board* const my_board, const struct move* const my_move) {
  int d_move_y = my_move->end/8-my_move->start/8;
  int d_move_x = my_move->end%8-my_move->start%8;

  if (d_move_x != 0 && d_move_y != 0) {
    // printf("Wrong angle!\n");
    return false;
  }

  int dx = (d_move_x != 0) ? (d_move_x > 0) ? 1 : -1 : 0;
  int dy = (d_move_y != 0) ? (d_move_y > 0) ? 8 : -8 : 0;

  for (int pos = my_move->start+dx+dy; pos != my_move->end; pos += dx+dy) {
    if (my_board->piece[pos] != empty) {
      return false;
    }
  }
  return true;
}

bool is_legal_queen(const struct board* const my_board, const struct move* const my_move) {
  int d_move_y = my_move->end/8-my_move->start/8;
  int d_move_x = my_move->end%8-my_move->start%8;

  if ((abs(d_move_y) != abs(d_move_x)) && (d_move_x != 0 && d_move_y != 0)) {
    // printf("Wrong angle!\n");
    return false;
  }

  int dx = (d_move_x != 0) ? (d_move_x > 0) ? 1 : -1 : 0;
  int dy = (d_move_y != 0) ? (d_move_y > 0) ? 8 : -8 : 0;

  for (int pos = my_move->start+dx+dy; pos != my_move->end; pos += dx+dy) {
    if (my_board->piece[pos] != empty) {
      return false;
    }
  }
  return true;
}

bool is_legal_king(const struct board* const my_board, const struct move* const my_move) {
  int d_move_y = my_move->end/8-my_move->start/8;
  int d_move_x = my_move->end%8-my_move->start%8;

  if (abs(d_move_y) <= 1 && abs(d_move_x) <= 1) {
    return true;
  }
  
  if (is_white(my_board->piece[my_move->start])) {
    if (my_move->end == 58) {
      if (!(my_board->state & 0b000010) && !(my_board->state & 0b000100)) {
        if (my_board->piece[57] == empty && my_board->piece[58] == empty && my_board->piece[59] == empty) {
          return true;
        }
      }
    }
    if (my_move->end == 62) {
      if (!(my_board->state & 0b000001) && !(my_board->state & 0b000100)) {
        if (my_board->piece[61] == empty && my_board->piece[62] == empty) {
          return true;
        }
      }
    }
  } else {
    if (my_move->end == 2) {
      if (!(my_board->state & 0b010000) && !(my_board->state & 0b100000)) {
        if (my_board->piece[1] == empty && my_board->piece[2] == empty && my_board->piece[3] == empty) {
          return true;
        }
      }
    }
    if (my_move->end == 6) {
      if (!(my_board->state & 0b001000) && !(my_board->state & 0b100000)) {
        if (my_board->piece[5] == empty && my_board->piece[6] == empty) {
          return true;
        }
      }
    }
  }
  
  return false;
}

bool is_legal_move(const struct board* const my_board, const struct move* const my_move, const int turn) {
  if (is_white(my_board->piece[my_move->start]) != (turn == white)) {
    // printf("This isn't your piece!\n");
    return false;
  }
  if (my_board->piece[my_move->end] != empty && is_white(my_board->piece[my_move->end]) == (turn == white)) {
    // printf("You can't capture your own piece!\n");
    return false;
  }
  if (my_move->start == my_move->end) {
    // printf("You must move!\n");
    return false;
  }
  switch (my_board->piece[my_move->start]%8) {
    case wp:
      return is_legal_pawn(my_board, my_move);
    case wn:
      return is_legal_knight(my_board, my_move);
    case wb:
      return is_legal_bishop(my_board, my_move);
    case wr:
      return is_legal_rook(my_board, my_move);
    case wq:
      return is_legal_queen(my_board, my_move);
    case wk:
      return is_legal_king(my_board, my_move);
  }
  // printf("Piece not imlemented yet!\n");
  return false;
}

bool is_in_bounds(const char* const move_str) {
  return (move_str[0]-'a' >= 0) && (move_str[1]-'0' >= 1) && (move_str[0]-'a' < 8) && (move_str[1]-'0' < 9);
}

void make_move(struct board* const my_board, const struct move* const my_move) {
  // White en passant:
  if (my_board->piece[my_move->start] == wp && my_board->piece[my_move->end] == empty && abs(my_move->end%8 - my_move->start%8) == 1) {
    my_board->piece[my_move->end+8] = empty;
  }
  // Black en passant:
  if (my_board->piece[my_move->start] == bp && my_board->piece[my_move->end] == empty && abs(my_move->end%8 - my_move->start%8) == 1) {
    my_board->piece[my_move->end-8] = empty;
  }
  // Normal move logic:
  my_board->piece[my_move->end] = my_board->piece[my_move->start];
  my_board->piece[my_move->start] = empty;
  my_board->last_move = *my_move;
  // Set flags for board state:
  switch (my_board->piece[my_move->end]) {
    case wr:
      if (my_move->start/8 == 7 && my_move->start%8 == 0) {
        my_board->state |= 0b000010;
      }
      if (my_move->start/8 == 7 && my_move->start%8 == 7) {
        my_board->state |= 0b000001;
      }
      break;
    case wk:
      my_board->state |= 0b000100;
      break;    
    case br:
      if (my_move->start/8 == 0 && my_move->start%8 == 0) {
        my_board->state |= 0b010000;
      }
      if (my_move->start/8 == 0 && my_move->start%8 == 7) {
        my_board->state |= 0b001000;
      }
      break;    
    case bk:
      my_board->state |= 0b100000;
      break;  
  }
  // White pawn promotion:
  if (my_board->piece[my_move->end] == wp && my_move->end/8 == 0) {
    my_board->piece[my_move->end] = wq;
  }
  // Black pawn promotion:
  if (my_board->piece[my_move->end] == bp && my_move->end/8 == 7) {
    my_board->piece[my_move->end] = bq;
  }
  // Long castle
  if (my_board->piece[my_move->end] == wk && my_move->end-my_move->start == -2) {
    my_board->piece[my_move->end-2] = empty;
    my_board->piece[my_move->end+1] = wr;
  }
  if (my_board->piece[my_move->end] == bk && my_move->end-my_move->start == -2) {
    my_board->piece[my_move->end-2] = empty;
    my_board->piece[my_move->end+1] = br;
  }
  // Short castle
  if (my_board->piece[my_move->end] == wk && my_move->end-my_move->start == 2) {
    my_board->piece[my_move->end+1] = empty;
    my_board->piece[my_move->end-1] = wr;
  }
  if (my_board->piece[my_move->end] == bk && my_move->end-my_move->start == 2) {
    my_board->piece[my_move->end+1] = empty;
    my_board->piece[my_move->end-1] = br;
  }
}

void undo_move(struct board* const my_board, const struct move* const my_move) {

}

int ask_for_turn() {
  int color;
  do {
    printf("Select color (1-White, 2-Black): ");
    scanf("%d", &color);
    if (color != 1 && color != 2) {
      printf("Wrong color, try again!\n");
    }
  } while (color != 1 && color != 2);

  return (color == 1) ? 1 : -1;
}

bool ask_for_move(const struct board* const my_board, struct move* const next_move, const int turn) {
  draw_board(my_board);
  char next_move_start[3];
  char next_move_end[3];
  if (turn == white) {
    printf("White ");
  } else {
    printf("Black ");
  }
  printf("moves: ");
  scanf("%2s", next_move_start);
  scanf("%2s", next_move_end);
  if (!is_in_bounds(next_move_start) || !is_in_bounds(next_move_end)) {
    printf("This move is out of bounds!\n\n");
    return false;
  }
  next_move->start = change_notation_to_int(next_move_start);
  next_move->end = change_notation_to_int(next_move_end);
  bool legal = is_legal_move(my_board, next_move, turn);
  if (!legal) {
    printf("This move isn't legal!\n\n");
  }
  return legal;
}

int generate_legal_moves_list(const struct board* const my_board, struct move* const moves_list, const int turn) {
  int idx = 0;
  int pieces_positions[16];
  int pieces_positions_size = 0;
  if (turn == white) {
    for (int pos = 0; pos < 64; pos++) {
      if (my_board->piece[pos] != empty && is_white(my_board->piece[pos])) {
        pieces_positions[pieces_positions_size] = pos;
        pieces_positions_size++;
      }
    }
  } else {
    for (int pos = 0; pos < 64; pos++) {
      if (my_board->piece[pos] != empty && !is_white(my_board->piece[pos])) {
        pieces_positions[pieces_positions_size] = pos;
        pieces_positions_size++;
      }
    }
  }
  for (int start_i = 0; start_i < pieces_positions_size; start_i++) {
    for (int end = 0; end < 64; end++) {
      struct move move_tmp = {pieces_positions[start_i], end};
      if (is_legal_move(my_board, &move_tmp, turn)) { 
        moves_list[idx] = move_tmp;
        idx++;
      }
    }
  }
  return idx;
}

bool is_check(const struct board* const my_board, const int turn) {
  struct move moves_list[256];
  const int moves_list_size = generate_legal_moves_list(my_board, moves_list, turn);
  for (int i = 0; i < moves_list_size; i++) {
    if (my_board->piece[moves_list[i].end]%8 == wk) {
      return true;
    }
  }
  return false;
}

bool is_checkmate(const struct board* const my_board, const int turn) {
  if (!is_check(my_board, turn)) {
    return false;
  }
  struct move moves_list[256];
  const int moves_list_size = generate_legal_moves_list(my_board, moves_list, -turn);
  int check_counter = 0;
  for (int i = 0; i < moves_list_size; i++) {
    struct board copied_board = *my_board;
    make_move(&copied_board, &moves_list[i]);
    if (is_check(&copied_board, turn)) {
      check_counter++;
    }
  }
  // If every viable move results in another check
  if (check_counter == moves_list_size) {
    return true;
  }
  return false;
}

void print_moves_list(const struct board* const my_board, const struct move* const moves_list, const int moves_list_size) {
  printf("Moves list:\n");
  char move_tmp_1[3];
  char move_tmp_2[3];
  for (int i = 0; i < moves_list_size; i++) {
    printf("- %s -> %s ", lookup_piece(my_board->piece[moves_list[i].start]), lookup_piece(my_board->piece[moves_list[i].end]));
    // printf("(%d -> %d) ", moves_list[i].start, moves_list[i].end);
    change_notation_to_str(moves_list[i].start, move_tmp_1);
    change_notation_to_str(moves_list[i].end, move_tmp_2);
    printf("(%s -> %s)\n", move_tmp_1, move_tmp_2);
  }
}

int eval(const struct board* const my_board, const struct move* const my_move, const int turn, const int depth, const int max_seq_depth) {
  if (depth > 0) {
    struct move next_move;
    int game_eval = engine_move(my_board, &next_move, -turn, depth-1, max_seq_depth-1); 
    return game_eval;
  }
  
  int score = 0;
  int non_pawn_material = 0;
  int white_king_pos = 0;
  int black_king_pos = 0;

  // Check all pieces
  for (int pos = 0; pos < 64; pos++) {
    int table_pos = (my_board->piece[pos] < 8) ? pos : ((7-pos/8)*8 + pos%8);
    switch (my_board->piece[pos]%8) {
    case wp:
      score += 100 * (is_white(my_board->piece[pos]) ? 1 : -1);
      score += pawn_value_table[table_pos] * (is_white(my_board->piece[pos]) ? 1 : -1);
      break;
    case wn:
      score += 320 * (is_white(my_board->piece[pos]) ? 1 : -1);
      score += knight_value_table[table_pos] * (is_white(my_board->piece[pos]) ? 1 : -1);
      non_pawn_material += 3;
      break;
    case wb:
      score += 325 * (is_white(my_board->piece[pos]) ? 1 : -1);
      score += bishop_value_table[table_pos] * (is_white(my_board->piece[pos]) ? 1 : -1);
      non_pawn_material += 3;
      break;
    case wr:
      score += 500 * (is_white(my_board->piece[pos]) ? 1 : -1);
      non_pawn_material += 5;
      break;
    case wq:
      score += 975 * (is_white(my_board->piece[pos]) ? 1 : -1);
      non_pawn_material += 9;
      break;
    case wk:
      score += 32767 * (is_white(my_board->piece[pos]) ? 1 : -1);
      if (my_board->piece[pos] < 8) {
        white_king_pos = pos;
      } else {
        black_king_pos = (7-pos/8)*8 + pos%8;
      }
      break;
    }
  }
  
  if (non_pawn_material <= 14) {
    score += king_middlegame_value[white_king_pos];
    score -= king_middlegame_value[black_king_pos];
  } else {
    score += king_endgame_value[white_king_pos];
    score -= king_endgame_value[black_king_pos];
  }

  // Pos Score -> White is winnig
  // Neg Score -> Back is winning
  return score;
}

int engine_move(const struct board* const my_board, struct move* const next_move, const int turn, const int depth, const int max_seq_depth) {
  struct move moves_list[256];
  const int moves_list_size = generate_legal_moves_list(my_board, moves_list, turn);
  int best_i = 0;
  int best_eval = (turn == white) ? INT_MIN : INT_MAX;
  for (int i = 0; i < moves_list_size; i++) {
    struct board copied_board = *my_board;
    make_move(&copied_board, &moves_list[i]);
    int current_eval;
    if (my_board->piece[moves_list[i].end] != empty && depth == 0 && max_seq_depth >= 0) {
      // Continue while in a capture sequence (ON)
      current_eval = eval(&copied_board, &(moves_list[i]), turn, 1, max_seq_depth);
    } else {
      current_eval = eval(&copied_board, &(moves_list[i]), turn, depth, max_seq_depth);
    }
    if (turn == white && current_eval > best_eval) {
      best_eval = current_eval;
      best_i = i;
    }
    if (turn == black && current_eval < best_eval) {
      best_eval = current_eval;
      best_i = i;
    }
  }
  *next_move = moves_list[best_i];
  // printf("Depth = %d => Eval = %d\n", depth, best_eval);
  return best_eval;
}


// Main function:

int main() {
  printf("Chess started!\n");

  struct board main_board;
  init_board(&main_board);

  int turn = white;
  int engine_turn = -ask_for_turn();

  while (true) {
    struct move next_move;
    bool legal = false;
    while (!legal) {
      // struct move moves_list[256];
      // const int moves_list_size = generate_legal_moves_list(&main_board, moves_list, turn);
      // print_moves_list(&main_board, moves_list, moves_list_size);
      if (turn != engine_turn) {
        legal = ask_for_move(&main_board, &next_move, turn);
      } else {
        draw_board(&main_board);
        printf("Engine moves: ");
        clock_t time_before_move = clock();
        int game_eval = engine_move(&main_board, &next_move, turn, 3, 4); 
        clock_t time_after_move = clock();
        clock_t difference_in_ms = (time_after_move-time_before_move) * 1000 / CLOCKS_PER_SEC;
        char move_tmp_1[3];
        char move_tmp_2[3];
        change_notation_to_str(next_move.start, move_tmp_1);
        change_notation_to_str(next_move.end, move_tmp_2);
        printf("%s %s\n\n", move_tmp_1, move_tmp_2);
        printf("Engine thought for %lums\n", difference_in_ms);
        printf("Eval = %d\n\n", game_eval);
        legal = true;
      }
    }
    make_move(&main_board, &next_move);

    if (is_check(&main_board, turn)) {
      printf("Check!\n");
    }
    if (is_checkmate(&main_board, turn)) {
      printf("Chceckmate!\n");
        if (turn == white) {
        printf("White ");
      } else {
        printf("Black ");
      }
      printf("wins!\n");
      return 0;
    }

    turn = -turn;
  }

  return 0;
}
