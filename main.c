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
      for( uint8_t col = 3; col > 0; )
        if( board[row][col] == 0 ) {
          moves++;
          for( uint8_t moveCol = col; moveCol > 0; moveCol-- )
            board[row][moveCol] = board[row][moveCol-1];
          board[row][0] = 0;
        } else
          col--;

  if( direction == left )
    for( uint8_t row = 0; row < 4; row++ )
      for( uint8_t col = 0; col < 3; )
        if( board[row][col] == 0 ) {
          moves++;
          for( uint8_t moveCol = col; moveCol < 3; moveCol++ )
            board[row][moveCol] = board[row][moveCol+1];
          board[row][3] = 0;
        } else
          col++;

  if( direction == up )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 0; row < 3; )
        if( board[row][col] == 0 ) {
          moves++;
          for( uint8_t moveRow = row; moveRow < 3; moveRow++ )
            board[moveRow][col] = board[moveRow+1][col];
          board[3][col] = 0;
        } else
          row++;

  if( direction == down )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 3; row > 0; )
        if( board[row][col] == 0 ) {
          moves++;
          for( uint8_t moveRow = row; moveRow > 0; moveRow-- )
            board[moveRow][col] = board[moveRow-1][col];
          board[0][col] = 0;
        } else
          row--;

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
          scoreDelta += pow(2, board[row][col]);
        }

  if( direction == left )
    for( uint8_t row = 0; row < 4; row++ )
      for( uint8_t col = 0; col < 3; col++ )
        if( board[row][col] > 0 && board[row][col+1] == board[row][col] ) {
          board[row][col]++;
          board[row][col+1] = 0;
          scoreDelta += pow(2, board[row][col]);
        }

  if( direction == up )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 0; row < 3; row++ )
        if( board[row][col] > 0 && board[row+1][col] == board[row][col] ) {
          board[row][col]++;
          board[row+1][col] = 0;
          scoreDelta += pow(2, board[row][col]);
        }

  if( direction == down )
    for( uint8_t col = 0; col < 4; col++ )
      for( uint8_t row = 3; row > 0; row-- )
        if( board[row][col] > 0 && board[row-1][col] == board[row][col] ) {
          board[row][col]++;
          board[row-1][col] = 0;
          scoreDelta += pow(2, board[row][col]);
        }
  return scoreDelta;
}

void handleTiles( dir_t direction ) {
  uint8_t moves = moveTiles( direction );
  score += mergeTiles( direction );
  moves += moveTiles( direction );
  if( moves )
    spawnTile();
}

void game() {
  for( uint8_t i = 0; i < 16; i++ )
    ((uint8_t*)board)[i] = 0;
  score = 0;

  spawnTile();
  spawnTile();

  uint8_t canWin = 1;
  while( canWin ) {
    drawBoard();
    if( BTN_UP )
      handleTiles(up);
    if( BTN_DOWN )
      handleTiles(down);
    if( BTN_RIGHT )
      handleTiles(right);
    if( BTN_LEFT )
      handleTiles(left);
    while( (PINC | 0xf0) != 0xff );
    _delay_ms(50); //lazy debouncing
  }
}

void mainMenu() {
  const uchar text1[] = " 2048 AVR EDITION   ";
  const uchar text2[] = " Press any button!  ";
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

  while( (PINC | 0xf0) == 0xff ); //wait for any button

  TCCR1B = 0; //stop timer
  seed = (uint8_t)TCNT1; //use lower bits as seed for prng
  seed_a = (uint8_t)(TCNT1>>8);
  game();
}

int main() {
  //init PORTC (buttons on PC0-3, debug led on PC5)
  DDRC = 0b00100000;
  PORTC = 0b00001111;

/*  while(1) {
   if( BTN_UP )
     PORTC ^= 0b00100000;
   if( BTN_LEFT )
     PORTC ^= 0b00100000;
   if( BTN_RIGHT )
     PORTC ^= 0b00100000;
   if( BTN_DOWN )
     PORTC ^= 0b00100000;
  }*/

  init();
  setMode1(true,false); //cursor increment on, display shifting off
  setMode2(true,false,false); //display on, cursor off, cursorFlash off
  setMode3(true,true,false); //8-bit mode, multi-line display, small font

  while( 1 ) {
    mainMenu();
  }

  return 0;
}
