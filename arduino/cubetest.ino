#include <stdint.h>

#define SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ISSET(v, n) ((v >> n) & 1)

#define DIM 5

/* PINS */
#define RESET 22
#define DATA 24

#define WD0 23
#define WD1 25
#define WD2 27
#define WD3 29
#define WD4 31

#define COL_ADDR0 34
#define COL_ADDR1 36
#define COL_ADDR2 38

#define LAY0 44
#define LAY1 46
#define LAY2 48
#define LAY3 50
#define LAY4 52

typedef uint8_t Matrix[DIM][DIM][DIM];
typedef bool (*AnimationFunc)(Matrix m, int state);
typedef struct animationEngine {
  uint32_t timeBuffer;

  uint32_t state;
  uint8_t speed;
  uint16_t duration;
  uint32_t animationIterations;

  AnimationFunc currentFunc;
} AnimationEngine;

Matrix mat;

int out_pins[] = { RESET, DATA, WD0, WD1, WD2, WD3, WD4, COL_ADDR0, COL_ADDR1, COL_ADDR2, LAY0, LAY1, LAY2, LAY3, LAY4 };
int layers[] = { LAY0, LAY1, LAY2, LAY3, LAY4 };
int registers[] { WD0, WD1, WD2, WD3, WD4 };
int led_addrs[] = { 0b000, 0b001, 0b010, 0b011, 0b100 };
AnimationEngine eng;
uint32_t currentTime;

void set(int pin, int val) {
  digitalWrite(pin, val);
}


void setup() {
  Serial.begin(9600);
  int i = 0;
  for (i = 0; i < SIZE(out_pins); i++) {
    pinMode(out_pins[i], OUTPUT);
  }

  set(WD0, 1);
  set(WD1, 1);
  set(WD2, 1);
  set(WD3, 1);
  set(WD4, 1);
  set(COL_ADDR0, 0);
  set(COL_ADDR1, 0);
  set(COL_ADDR2, 0);
  set(LAY0, 0);
  set(LAY1, 0);
  set(LAY2, 0);
  set(LAY3, 0);
  set(LAY4, 0);
    
  pinMode(LED_BUILTIN, OUTPUT);
  int v = 0;
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        mat[x][y][z] = 0;
      }
    }
  }

  set(WD0, 1);
  set(WD1, 1);
  set(WD2, 1);
  set(WD3, 1);
  set(WD4, 1);
  set(RESET, 1);
  delay(1);
  set(RESET, 0);
  
  reset(&eng);
  currentTime = millis();
}

// z - layers
// y - bits
// x - register
void render(Matrix m) {
  set(RESET, 0);
  for (int z = 0; z < DIM; z++) {
    
    for (int x = 0; x < DIM; x++) {
      for (int y = 0; y < DIM; y++) {
        set(DATA, !m[x][y][z]);
        int addr = led_addrs[y];
                        
        set(COL_ADDR0, ISSET(addr, 0));
        set(COL_ADDR1, ISSET(addr, 1));
        set(COL_ADDR2, ISSET(addr, 2));
    
        set(registers[x], 0);
        set(registers[x], 1);
      }
    }
    set(layers[z], 1);
    delayMicroseconds(3000);
    //delay(1000);
    set(layers[z], 0);
  }
}

void clear(Matrix m) {
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = 0;
      }
    }
  }
}

AnimationFunc animations[] = { edges, movingDot };

AnimationFunc getRandomAnimationFunc() {
  const int nfunc = SIZE(animations);
  int i = random(nfunc);
  return animations[i];
}

void reset(AnimationEngine* e) {
   e->currentFunc = getRandomAnimationFunc();
   e->speed = random(4)+ 1;
   e->duration = random(7000) + 3000;
   e->timeBuffer = 0;
   e->state = 0;
   e->animationIterations = 0;
}

void tick(AnimationEngine* e, uint32_t delta, Matrix m) {
  if (e->currentFunc == NULL) {
    reset(e);
  }
 
  e->timeBuffer += delta;

  if (e->timeBuffer > e->duration && e->animationIterations > 0) {
    reset(e);
  }
  
  bool finished = e->currentFunc(m, e->state);
  e->state++;
  Serial.println(e->speed);
  if (finished) {
    e->animationIterations++;
  }
  
}

void movingDot(Matrix m, int state) {
  clear(m);
  m[state % DIM][0][0] = 1;

  return state != 0 && state % DIM == 0;
}

void edges(Matrix m, int state) {
  clear(m);
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = 
          (x % (DIM - 1) == 0) && (y % (DIM - 1) == 0) ||
          (y % (DIM - 1) == 0) && (z % (DIM - 1) == 0) ||
          (z % (DIM - 1) == 0) && (x % (DIM - 1) == 0);
          
      }
    }
  }
  return true;
}



void loop() {
  uint32_t now = millis();
  uint32_t delta = now - currentTime;
  currentTime = now;
  
  tick(&eng, delta, mat);
    
  render(mat);
}
