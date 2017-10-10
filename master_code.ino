
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include<Wire.h>

using namespace std;

/* 
 *  This version uses 1D arrays to save memory space
 *  Attribute POSITION is defined as position=column index*size+ row index
 *  POSITION HAS TO BE INITIALIZED FROM 0 TO 15
 *  /!\ with size=sqrt(list length) /!\
 *  For given position, row=position%size and column=position/size
*/

/* 
 *  COLOR CODE: 1=red, 2=green, 3=blue 4=yellow 0=NONE
 *  REQUESTED DATA FORMAT : 3 characters in [color,adress] where address takes 2 characters from 01 to 16 
 *  e.g. ['1','1','2'] = red on position 12
 *  FINAL VERSION !!!!
*/

int*tab=new int[16]; //actual grid
int*validity=new int[16]; //validity analysis 0=none, 1=valid or nothing(nothing valid by default)
#define speakerPin A15 

void setup() {
  Wire.end();
  Serial.begin(9600);
  Wire.begin();
  //fire up initial grid
  tab=initializeSudoku();
  
  // light up the initial position
  for(int i=0;i<16;i++){
    validity[i]=1;
    Wire.beginTransmission(i+1); 
    Wire.write(tab[i]);
    Wire.endTransmission(i+1);
    delay(100);
  }
  displaySudoku(tab,4);

}

void loop() {
  Serial.print("loop begin");

for(int i=1;i<17;i++){
  Serial.println("");
  Wire.requestFrom(i,3);
  char buf[3];
  int j=0;
  while (Wire.available()){ //might send less than expected
    char c=Wire.read();
    buf[j]=c;
    j++;
    }    
  evaluate(buf); //check validity of position
  delay(100); //TODO: modify delay?
  }
takeAction(); 
Serial.print("loop end");
}
  

bool isInList(int* list, int value, int listSize) {
  // test for presence of value in list
  for (int i = 0; i < listSize; i++) {
    if (list[i] == value) return true;
  }
  return false;
}

bool isOnRow(int position, int value, int* grid, int size){
  //line presence test
  int rowIndex = position%size;
  for (int i = rowIndex; i < size*size; i += size) {
    if (grid[i] == value && i!=position) return true;
  }
  return false;
}

bool isOnColumn(int position, int value, int* grid, int size) {
  //column presence test
  int lineIndex = position / size;
  for (int i = 0; i < size; i++) {
    if (grid[lineIndex*size + i] == value && (lineIndex*size + i)!=position) return true;
  }
  return false;
}

bool isOnBlock(int position, int value, int* grid, int size) {
  int const blockSize = 2;
  // INTENDED FOR 4*4 GRID ONLY
  // Modify accordingly for other dimensions
  // block presence test
  int blockPosition = blockSize*(int)((int)(position / size) / blockSize) + (int)((position%size) / blockSize);
  int slide = 0;
  switch (blockPosition) {
  case 1:
    slide = 2;
    break;
  case 2:
    slide = 8;
    break;
  case 3:
    slide = 10;
    break;
  }
  for (int i = 0; i < blockSize; i++) {
    if (grid[slide + i] == value && (slide + i)!=position) return true;
    if (grid[slide + i + size] == value && (slide+i+size)!=position) return true;
  }
  return false;
}

void displaySudoku(int* grid, int size) {
  //displays content of grid on screen
  //test method
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      Serial.print(grid[j+i*size]);
    }
    Serial.println("");
  }
  Serial.println("DONE");
}
int* copyGrid(int* grid, int size) {
  //copies a grid to a "virtual" one
  int *newGrid = new int[size*size];
  for (int i = 0; i < size*size; i++) {
    newGrid[i] = grid[i];
  }
  return newGrid;
}

bool backTrack(int* grid, int position, int size) { 
  //solve grid using backtrack method
  //where position=column index*size+row index
  if (position == size*size) {
    return true;
  }//all grid has been screened, job done
  if (grid[position] != 0) {
    return backTrack(grid, position + 1, size); //moving on if already filled
  }
  for (int i = 1; i < size + 1; i++) { //recursive part
    if (!isOnColumn(position, i, grid, size)&& !isOnRow(position, i, grid, size) && !isOnBlock(position, i, grid, size)) {
      grid[position] = i;
      if (backTrack(grid, position + 1, size)){
        return true;
      }
    }
  }
  grid[position] = 0;
  return false;
}

bool backTrackWithCondition(int* grid, int position, int size, int forbiddenValue, int forbiddenPosition) {
  //solver with condition (one specific value at a specific position is forbidden)
  //where position=column index*size+row index
  //used to see if there is more than one solution for a given position
  if (position == size*size) {
    return true;
  }//all grid has been screened, job done
  if (grid[position] != 0) {
    return backTrackWithCondition(grid, position + 1, size, forbiddenValue, forbiddenPosition); //moving on if already filled
  }
  for (int i = 1; i < size + 1; i++) { //recursive part
    if (!(i == forbiddenValue) || !(position == forbiddenPosition)) {
      {
        if (!isOnColumn(position, i, grid, size) && !isOnRow(position, i, grid, size) && !isOnBlock(position, i, grid, size)) {
          grid[position] = i;
          if (backTrackWithCondition(grid, position + 1, size, forbiddenValue, forbiddenPosition)) {
            return true;
          }
        }
      }
    }
  }
  grid[position] = 0;
  return false;
}


int* generateSudoku(int size) {
  //create empty grid
  int *grid = new int[16];
  for (int i = 0; i < size*size; i++) {
    grid[i] = 0;
  }
  return grid;
}

void addRandomNumber(int position, int* grid, int size) {
  //add random number to specified position respecting sudoku rules
  srand(analogRead(0));
  int value = 0;
  do {
    value = rand() % size + 1;
  } while (isOnBlock(position, value, grid, size) || isOnRow(position, value, grid, size) || isOnColumn(position, value, grid, size));
  grid[position] = value;
}

int* createRandomFilled(int size) {
  //create a random valid grid fully filled
  int* grid = generateSudoku(size);
  for (int position = 0; position < size*size; position++) {
    do {
      addRandomNumber(position, grid, size);
    } while (!backTrack(copyGrid(grid, size), 0, size));
  }
  return grid;
}

int generateRandom(int max) {
  // generate random int in (0, max)
  srand(analogRead(0));
  return rand() % (max - 1);
}

int* clearSudoku(int* validGrid,int size) {
  // progressively clears a previously fully filled valid grid
  // stops when you cannot take out any additional numbers without breaking the unicity of the solution
  // (so there is no need for guessing during solving)
  bool pursue = true;
  int toRemove = 0;
  int toStore = 0; //store the removed value if backup need
  int* alreadyTried = new int[16];
  int storeIndex = 0;
  while (pursue) {
    do {
      toRemove = generateRandom(16);
    } while (isInList(alreadyTried,toRemove,16));
    toStore = validGrid[toRemove]; //prepare backup
    validGrid[toRemove] = 0;
    
    if (backTrackWithCondition(copyGrid(validGrid, size), 0, size, toStore, toRemove)) {
      validGrid[toRemove] = toStore;
      break;
    }
    alreadyTried[storeIndex] = toRemove;
    storeIndex++;
    Serial.print("");
  }
  return validGrid;
}

int* initializeSudoku(){
  // creates the starting position
  int* temp=new int[16];
  temp=createRandomFilled(4); //we always want a 4*4 grid anyway
  temp=clearSudoku(temp,4);
  return temp;
}

void evaluate(char c[]){
  /*
   * pos from 0 to 15, value from 0 to 4, 0 being none
   * this function updates the validity grid
   */
  int value=(int) (c[0]-'0')%48; //to int
  int posHigh=(int)(c[1]-'0')%48;
  int posLow=(int)(c[2]-'0')%48;
  int pos=posLow+10*posHigh-1;
  if(value!=0){
    if(!isOnRow(pos,value,tab,4)&&!isOnColumn(pos,value,tab,4)&&!isOnBlock(pos,value,tab,4)){
      validity[pos]=1;
    } else{
      validity[pos]=0;
    }
  } else{
    validity[pos]=1; //if nothing then it is valid anyway
  }
 tab[pos]=value;
}

bool fullValid(){
  for(int i=0;i<16;i++){
    if (validity[i]==0) return false;
  }
  return true;
}

bool isOver(){
  for(int i=0;i<16;i++){
    if (tab[i]==0) return false;
  }
  return true;
}
void takeAction(){
  if(!fullValid()) {
    Serial.println("WRONG");
    tone(speakerPin, 277, 100);
    delay(120);
    tone(speakerPin, 277, 500);
    delay(500);
  }
  if(fullValid()&&isOver()){
    tone(speakerPin, 1760, 200);
    delay(100);
    tone(speakerPin, 1397, 200);
    delay(100);
    tone(speakerPin, 1760, 200);
    delay(100);
    tone(speakerPin, 1397, 500);
    delay(500);
  }
  //TODO: turn on buzzer
}

