#include <stdint.h>

#define SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ISSET(v, n) ((v >> n) & 1)

#define DIM 5

/* PINS */
#define RESET 32
#define DATA 30

#define WD0 34
#define WD1 36
#define WD2 38
#define WD3 40
#define WD4 42

#define COL_ADDR0 28
#define COL_ADDR1 26
#define COL_ADDR2 24

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
  randomSeed(analogRead(0));
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


AnimationFunc animations[] = { 
  flash, edges, cross_faces, faces, random_dots, icycles, ball, rotating_plane, lazers, bar_graph, chess3d, pillar, crosses };

AnimationFunc getRandomAnimationFunc() {
  int i = random(SIZE(animations));
  return animations[i];
}

void reset(AnimationEngine* e) {
  e->currentFunc = getRandomAnimationFunc();
  e->speed = random(3)+ 1;
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
  int newIterations = e->currentFunc(m, e->state);
  if (e->timeBuffer > e->duration && e->animationIterations != newIterations) {
    reset(e);
  }
  
  e->animationIterations = newIterations;
  e->state++;  
}

int crosses(Matrix m, int state) {
  int scaledState = state / eng.speed;
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = (x == z || x == (4 - z)) && (y == 0 || y == 4) && scaledState % 2 == 0;
      }
    }
  }

  return scaledState;
}

int pillar(Matrix m, int state) {
  int scaledState = state / eng.speed / 5;
  int iteration = scaledState % 10;
  int pillar = (scaledState / 10) % 3 + 1;
  float mid = DIM / 2;

  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = sqrt((mid - x)*(mid - x) + (mid - y)*(mid - y)) < pillar && z < pyramid_period_func(iteration, 5);
      }
    }
  }

  return scaledState / 10;
}

int chess3d(Matrix m, int state) {
  int scaledState = state / eng.speed / 10;
  int i = 0;
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        i++;
        m[x][y][z] = (i + scaledState) % 2;
      }
    }
  }
 
  return scaledState;
}

int8_t bars[DIM][DIM];
bool randomizedBarsThisRound = false;
int bar_graph(Matrix m, int state) {
  const int nstates = 9;
  int scaledState = state / 3 / eng.speed;

  int iteration = scaledState % nstates;
  if (iteration == 0 && !randomizedBarsThisRound) {
    randomizedBarsThisRound = true;
    for (int x = 0; x < DIM; x++) {
      for (int y = 0; y < DIM; y++) {
        bars[x][y] = max(random() % (DIM - 1), 0) - 2;
      } 
    }    
  } 
  if (iteration != 0) {
    randomizedBarsThisRound = false;
  }
    
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        if (bars[x][y] >= 0) {
          m[x][y][z] = (bars[x][y] + pyramid_period_func(iteration, 4)) > z;
        } else {
          m[x][y][z] = 0;
        }
        
      }
    }
  }

  return state / 3 / eng.speed / nstates;
}

int flash(Matrix m, int state) {
  int scaledState = state / 100;
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = scaledState % 2 == 0;
      }
    }
  }
  return scaledState % 2;
}

int random_dots(Matrix m, int state) {
  int growthBrake = 20;
  int maxGrowth = 20;
  int speedFactor = (eng.speed);
  int scaledState = (state / growthBrake / speedFactor);
  
  clear(m);

  for (int i = 0; i <= scaledState; i++) {
    m[random(5)][random(5)][random(5)] = 1;  
  }
  

  return scaledState / maxGrowth;
}

int lazerX = 0;
int lazerY = 0;
bool lazerRandomizedThisRound = false;
int lazers(Matrix m, int state) {
  const int nstates = 10;
  int speedFactor = eng.speed;
  int scaledState = state / speedFactor % nstates;

  if (scaledState == 0 && !lazerRandomizedThisRound) {
    lazerX = random(5);
    lazerY = random(5);
    lazerRandomizedThisRound = true;
  }
  if (scaledState != 0) {
    lazerRandomizedThisRound = false;
  }
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = x == lazerX && y == lazerY;
      }
    }
  }

  return state / speedFactor / nstates;
}

int rotating_plane(Matrix m, int state) {
  const int nstates = 3;
  int speedFactor = eng.speed * 5;
  int scaledState = state / speedFactor % nstates;
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        switch(scaledState) {
          case 0: m[x][y][z] = x == y; break;
          case 1: m[x][y][z] = x == z; break;
          case 2: m[x][y][z] = z == y; break;
        }
        
      }
    }
  }

  return state / speedFactor / nstates;
}

int icyX = 0;
int icyY = 0;
bool icyRandomizedThisRound = false;
int icycles(Matrix m, int state) {
  const int nstates = 5;
  int speedFactor = eng.speed * 10;
  int scaledState = (state / speedFactor) % nstates;

  if (scaledState == 0 && !icyRandomizedThisRound) {
    icyX = random(5);
    icyY = random(5);
    icyRandomizedThisRound = true;
  }
  if (scaledState != 0) {
    icyRandomizedThisRound = false;
  }
 
  clear(m);
  m[icyX][icyY][4 - scaledState] = 1;

  return state / speedFactor / nstates;
}

int edges(Matrix m, int state) {
  int speedFactor = eng.speed * 3;
  int scaledState = state / speedFactor % 2;
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        m[x][y][z] = scaledState && (
          (x % (DIM - 1) == 0) && (y % (DIM - 1) == 0) ||
          (y % (DIM - 1) == 0) && (z % (DIM - 1) == 0) ||
          (z % (DIM - 1) == 0) && (x % (DIM - 1) == 0)); 
          
      }
    }
  }
  
  return state / speedFactor / 2;
}

int faces(Matrix m, int state) {
  const int nstates = 27;
  int speedFactor = 
  (6 - eng.speed);
  int scaledState = (state / speedFactor) % nstates;
      
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        int target = x;
        if (scaledState < 9) {
          target = z;
        } else if (scaledState < 18) {
          target = y;
        } else if (scaledState < 27) {
          target = x;
        }

        m[x][y][z] = target == floor(pyramid_period_func(scaledState, 4.0));
      }
    }
  }

  return state / speedFactor / nstates;
}

int cross_faces(Matrix m, int state) {
  const int nstates = 9;
  int speedFactor = (6 - eng.speed);
  int scaledState = (state / speedFactor) % nstates;
    
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        int target;
        if (scaledState < 5) {
          target = (x + y) / 2;
        } else if (scaledState < 10) {
          target = (x + z) / 2;
        } 

        m[x][y][z] = target == floor(pyramid_period_func(scaledState, 4.0));
      }
    }
  }

  return state / speedFactor / nstates;
}

int ball(Matrix m, int state) {
  const int nstates = 4;
  const float mid = 5 / 2;
  int speedFactor = eng.speed * 3;
  int scaledState = (state / speedFactor) % nstates;
  Serial.println(scaledState);
  
  for (int x = 0; x < DIM; x++) {
    for (int y = 0; y < DIM; y++) {
      for (int z = 0; z < DIM; z++) {
        float distance = sqrt((mid - x)*(mid - x) + (mid - y)*(mid - y) + (mid - z)*(mid - z));
        m[x][y][z] = distance <= scaledState;
      }
    }
  }

  return state / speedFactor / nstates;
}

float pyramid_period_func(float x, float slope) {
  x = fmod(x, 2 * slope);
  if (x < slope) {
    return x;
  } else {
    return 2 * slope - x;
  }
}

void loop() {
  uint32_t now = millis();
  uint32_t delta = now - currentTime;
  currentTime = now;
  
  tick(&eng, delta, mat);
    
  render(mat);
}
