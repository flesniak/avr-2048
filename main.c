/* 2048 for 4x20 HD44780 displays *
 * 2014 Fabian Lesniak            */

#include <avr/io.h>
#include <util/delay.h>

#include "hd44780.h"

#define BTN_UP    !(PINC & (1 << PC0))
#define BTN_RIGHT !(PINC & (1 << PC3))
#define BTN_LEFT  !(PINC & (1 << PC1))
#define BTN_DOWN  !(PINC & (1 << PC2))

static uint8_t board[4][4];
static const char * const map[] = { "    ", "   2", "   4", "   8", "  16", "  32", "  64", " 128", " 256", " 512", "1024", "2048", "4096", "8192", " 16k", " 32k", " 64k" };
static uint32_t score = 0;
static uint8_t seed = 0xde;
static uint8_t seed_a = 0xad;

/*void int2num(uint32_t num, char* buffer, uint8_t length ) {
  for( uint8_t i = 0; i < length; i++ ) {
    uint8_t decades = 0;
    uint32_t dec = 1;
    for( uint8_t a = 0; a < length-i; a++ )
      dec *= 10;
    while( num > dec ) {
      decades++;
      num -= dec;
    }
    buffer[i] = 48+decades;
  }
}*/

void debounce() {
  while( (PINC | 0xf0) != 0xff )
    _delay_ms(10); //wait for button release
}

inline void wait4button() {
  while( (PINC | 0xf0) == 0xff ); //wait for any button
  debounce();
}

void drawBoard() {
  for( uchar row = 0; row < 4; row++ ) {
    switch( row ) {
      case 0: //setCursorStart();
              setDDRAMAddress(0);
              break;
      case 1: setDDRAMAddress(0x40);
              break;
      case 2: setDDRAMAddress(0x14);
              break;
      case 3: setDDRAMAddress(0x54);
              break;
    }
    for( uchar col = 0; col < 4; col++ ) {
      writeRAM('|');
      for( uchar chr = 0; chr < 4; chr++ )
        writeRAM( map[board[row][col]][chr] );
    }
  }
}

uint8_t rand() {
  seed^=seed<<3;
  seed^=seed>>5;
  seed^=seed_a++>>2;
  return seed;
}

void spawnTile() {
  uint8_t freeTiles = 0;
  for( uint8_t i = 0; i < 16; i++ )
    if( ((uint8_t*)board)[i] == 0 )
      freeTiles++;

  uint8_t position = rand() % freeTiles; //TODO perhaps change to something less calculation-time-intensive

  freeTiles--;
  for( uint8_t i = 0; i < 16; i++ )
    if( ((uint8_t*)board)[i] == 0 ) {
      if( freeTiles > position )
        freeTiles--;
      else {
        if( rand() > 230 ) //~0.9*256
          ((uint8_t*)board)[i] = 2;
        else
          ((uint8_t*)board)[i] = 1;
        return;
      }
    }
}

typedef enum { up, down, right, left } dir_t;

uint8_t moveTiles( dir_t direction ) {
  uint8_t moves = 0;
  if( direction == right )
    for( uint8_t row = 0; row < 4; row++ )
      for( uint8_t col = 1; col < 4; col++ )
        if( board[row][col] == 0 ) {
          for( uint8_t moveCol = col; moveCol > 0; moveCol-- ) {
            board[row][moveCol] = board[row][moveCol-1];
            if( board[row][moveCol] )
              moves++;
          }
          board[row][0] = 0;
        }

  if( direction == left )
    for( uint8_t row = 0; row < 4; row++ )
      for( int8_t col = 2; col >= 0; col-- )
        if( board[row][col] == 0 ) {
          for( uint8_t moveCol = col; moveCol < 3; moveCol++ ) {
            board[row][moveCol] = board[row][moveCol+1];
            if( board[row][moveCol] )
              moves++;
          }
          board[row][3] = 0;
        }

  if( direction == down )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 1; row < 4; row++ )
        if( board[row][col] == 0 ) {
          for( uint8_t moveRow = row; moveRow > 0; moveRow-- ) {
            board[moveRow][col] = board[moveRow-1][col];
            if( board[moveRow-1][col] )
              moves++;
          }
          board[0][col] = 0;
        }

  if( direction == up )
    for( uint8_t col = 0; col < 4; col++ )
      for( int8_t row = 2; row >= 0; row-- )
        if( board[row][col] == 0 ) {
          for( uint8_t moveRow = row; moveRow < 3; moveRow++ ) {
            board[moveRow][col] = board[moveRow+1][col];
            if( board[moveRow][col] )
              moves++;
          }
          board[3][col] = 0;
        }

  return moves;
}

uint32_t mergeTiles( dir_t direction ) {
  uint32_t scoreDelta = 0;
  if( direction == right )
    for( uint8_t row = 0; row < 4; row++ )
      for( uint8_t col = 3; col > 0; col-- )
        if( board[row][col] > 0 && board[row][col-1] == board[row][col] ) {
          board[row][col]++;
          board[row][col-1] = 0;
          scoreDelta += 1 << board[row][col];
        }

  if( direction == left )
    for( uint8_t row = 0; row < 4; row++ )
      for( uint8_t col = 0; col < 3; col++ )
        if( board[row][col] > 0 && board[row][col+1] == board[row][col] ) {
          board[row][col]++;
          board[row][col+1] = 0;
          scoreDelta += 1 << board[row][col];
        }

  if( direction == up )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 0; row < 3; row++ )
        if( board[row][col] > 0 && board[row+1][col] == board[row][col] ) {
          board[row][col]++;
          board[row+1][col] = 0;
          scoreDelta += 1 << board[row][col];
        }

  if( direction == down )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 3; row > 0; row-- )
        if( board[row][col] > 0 && board[row-1][col] == board[row][col] ) {
          board[row][col]++;
          board[row-1][col] = 0;
          scoreDelta += 1 << board[row][col];
        }
  return scoreDelta;
}

void handleTiles( dir_t direction ) {
  uint8_t moves = moveTiles( direction );
  uint32_t scoreDelta = mergeTiles( direction );
  moves += moveTiles( direction );
  if( moves || scoreDelta ) {
    _delay_ms(100);
    spawnTile();
  }
  score += scoreDelta;
  debounce();
  PORTC ^= 0b00100000;
}

bool gameOver() {
  for( uint8_t row = 0; row < 3; row++ )
    for( uint8_t col = 0; col < 3; col++ )
      if( board[row][col] == 0 ||
          board[row][col] == board[row+1][col] ||
          board[row][col] == board[row][col+1] )
        return false;
  for( uint8_t edge = 0; edge < 3; edge++ )
      if( board[3][edge] == 0 ||
          board[edge][3] == 0 ||
          board[3][edge] == board[3][edge+1] ||
          board[edge][3] == board[edge+1][3] )
        return false;
  if( board[3][3] == 0 )
    return false;
  return true;
}

void showScore() {
  const char text1[] = "  You lost! Score:  ";
  char text2[20] = "                   ";
  //int2num( score, text2, 6 );
  uint8_t decades = 0;
  uint32_t tScore = score;
  while( tScore > 100000 ) {
    decades++;
    tScore -= 100000;
  }
  text2[14] = '0'+decades;
  decades = 0;
  while( tScore >= 10000 ) {
    decades++;
    tScore -= 10000;
  }
  text2[15] = '0'+decades;
  decades = 0;
  while( tScore >= 1000 ) {
    decades++;
    tScore -= 1000;
  }
  text2[16] = '0'+decades;
  decades = 0;
  while( tScore >= 100 ) {
    decades++;
    tScore -= 100;
  }
  text2[17] = '0'+decades;
  decades = 0;
  while( tScore >= 10 ) {
    decades++;
    tScore -= 10;
  }
  text2[18] = '0'+decades;
  text2[19] = '0'+tScore;
  const char text3[] = "  Press any button  ";
  const char text4[] = "    to continue!    ";

  setDDRAMAddress(0x00);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text1[i]);
  }
  setDDRAMAddress(0x40);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text2[i]);
  }
  setDDRAMAddress(0x14);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text3[i]);
  }
  setDDRAMAddress(0x54);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text4[i]);
  }
  wait4button();
}


void game() {
  for( uint8_t i = 0; i < 16; i++ )
    ((uint8_t*)board)[i] = 0;
  score = 0;

  spawnTile();
  spawnTile();

/*  board[0][0] = 7;
  board[0][2] = 7;
  board[1][1] = 7;
  board[1][3] = 7;
  board[2][2] = 7;
  board[2][0] = 7;
  board[3][1] = 7;
  board[3][3] = 0;
  board[0][1] = 4;
  board[0][3] = 4;
  board[1][0] = 4;
  board[1][2] = 4;
  board[2][1] = 4;
  board[2][3] = 4;
  board[3][0] = 4;
  board[3][2] = 4;
  score = 133711;*/

  drawBoard();

  while( !gameOver() ) {
    if( BTN_UP )
      handleTiles(up);
    if( BTN_DOWN )
      handleTiles(down);
    if( BTN_RIGHT )
      handleTiles(right);
    if( BTN_LEFT )
      handleTiles(left);
    drawBoard();
  }
  wait4button();
  showScore();
}

void mainMenu() {
  const char text1[] = " 2048 AVR EDITION   ";
  const char text2[] = " Press any button!  ";
  clearDisplay();
  setDDRAMAddress(0x00);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text1[i]);
  }
  setDDRAMAddress(0x40);
  for( uchar i = 0; i < 20; i++ ) {
    writeRAM(text2[i]);
  }

  //get a seed for our random number generator using a timer
  TCCR1B = 0b001; //f_cpu/1

  wait4button();
  PORTC ^= 0b00100000;

  TCCR1B = 0; //stop timer
  seed = (uint8_t)TCNT1; //use lower bits as seed for prng
  seed_a = (uint8_t)(TCNT1>>8);
  game();
}

int main() {
  //init PORTC (buttons on PC0-3, debug led on PC5)
  DDRC = 0b00100000;
  PORTC = 0b00001111;

  init();
  setMode1(true,false); //cursor increment on, display shifting off
  setMode2(true,false,false); //display on, cursor off, cursorFlash off
  setMode3(true,true,false); //8-bit mode, multi-line display, small font

  while( 1 ) {
    mainMenu();
  }

  return 0;
}
