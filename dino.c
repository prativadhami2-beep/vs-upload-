#include <stdio.h> // For printf, scanf, FILE operations input output function 
#include <stdlib.h>// For rand, srand, exit functions,random numbers, and memory management
#include <windows.h>//window  console graphics and input handling
#include <conio.h>// For kbhit and getch functions, keyboard input handling
#include <time.h>//random speed for enemies
//constants

#define WIDTH 90
#define HEIGHT 28
//this define the game screen size 
#define MAX_ENEMIES 4
#define MAX_BULLETS 5
#define MAX_BOSS_BULLETS 6
//this shows the limit of number of enemys and bullets on the screen at once 
#define BASE_FPS 22
#define SHOOT_COOLDOWN 6
//control game speed and firing rate 

// Time-based leveling: seconds per level before advancing
#define SECONDS_PER_LEVEL 20 // each level lasts for 20 seconds 

// Score awarded per second, scales with level
#define SCORE_PER_SECOND_BASE 5

// -------- FUNCTION PROTOTYPES --------
void showGameOver();

// -------- STRUCTS --------
typedef struct{
    int x,y,speed,type;
}Enemy; //enemy struct with x and y position, speed, and type (for different sprites)

typedef struct{
    int x,y,active;
}Bullet; //represent bullets fired by player and boss, with position and active state

// -------- GLOBALS --------
Enemy enemies[MAX_ENEMIES];
Bullet bullets[MAX_BULLETS];
Bullet bossBullets[MAX_BOSS_BULLETS];
// these array stores all the  active enemies and bullets in the game
char screen[HEIGHT][WIDTH+1];

HANDLE console;

int dinoX=10;
int dinoY=HEIGHT-6;
//player position on the screen, starts near the bottom left
int velocity=0;
int gravity=1;
//controls the vertical movement of the player, simulating jumping and falling
int score=0;
int health=100;
int level=1;
// core game variables: score, health, and current level
int shootTimer=0;

int bossHP=200;
int bossX=65;
int bossY=8;
int bossDir=1;
int bossDirY=1;

int terrainOffset=0;

int bg1=0,bg2=0,bg3=0;

int highscore=0;

int gameSpeed=BASE_FPS;
int gameOver=0;

// ---- Time-based level tracking ----
// We count frames; at gameSpeed fps, SECONDS_PER_LEVEL*gameSpeed frames = next level
int frameCount=0;

// ---- Passive score accumulator ----
// We award fractional score per frame; accumulate to avoid int truncation
float scoreAccum=0.0f;

// -------- SPRITES --------
char *dino[4]={
"  __ ",
" (oo)",
"/|__|",
" /  \\"
};// ASCII  sprites 
//instead of images the game draws directly to the console using characters,


char *enemy1[3]={
" /V\\ ",
"(oo)",
"/||\\"
};

char *enemy2[3]={
" /W\\ ",
"(xx)",
"/||\\"
};

// -------- FUNCTIONS --------

void setLevelColor()//change console  color based on level for visual variety and feedback, 
{
if(level==1) SetConsoleTextAttribute(console,10);//1 = green
if(level==2) SetConsoleTextAttribute(console,11);//2 = cyan
if(level==3) SetConsoleTextAttribute(console,14);//3 = yellow
if(level==4) SetConsoleTextAttribute(console,13);//4 = magenta
if(level>=5) SetConsoleTextAttribute(console,12);//5 = red
}

void levelDialogue()//helps to setup the atmosphere and progression between levels 
{
system("cls");//clear the console for new level

printf("\n\nYou found another shard of your egg...\n");//this creates the game's core storyline ,
// the dinosaur is collecting fragments of it's stolen eggs throughout the journey 


// level based dialogues different texts apper depending on the current level 
if(level==1) printf("The journey begins.\n");/
if(level==2) printf("Enemies grow stronger.\n");
if(level==3) printf("The skies darken.\n");
if(level==4) printf("You feel the boss nearby.\n");
if(level==5) printf("The final shard awaits.\n");

printf("\nPress any key...");
getch();
}
//rebdering system = how graphics appear on screen 
void clearBuffer()//reset the screen every frame by filling it with spaces and null terminators
{
for(int y=0;y<HEIGHT;y++)
{
for(int x=0;x<WIDTH;x++)
screen[y][x]=' ';
screen[y][WIDTH]='\0';
}
}
//draw text 
void drawText(int x,int y,char *text)//places text at a position on the screen buffer, checking bounds to avoid overflow
{
for(int i=0;text[i]!=0;i++)
if(x+i>=0 && x+i<WIDTH && y>=0 && y<HEIGHT)
screen[y][x+i]=text[i];
}
//draw sprites
void drawSprite(char **sprite,int h,int x,int y)//draws multiline ASCII graphics 
{
for(int i=0;i<h;i++)
if(y+i>=0 && y+i<HEIGHT)
drawText(x,y+i,sprite[i]);
}
// render function 
void render() //prints the entire screen buffer ,the game 1st draw everything into memory and diplay it all at once to avoid flickering and improve performance
{
COORD c={0,0};
SetConsoleCursorPosition(console,c);

for(int y=0;y<HEIGHT;y++)
printf("%s\n",screen[y]);
}

void hideCursor()
{
CONSOLE_CURSOR_INFO ci;
ci.dwSize=1;
ci.bVisible=FALSE;
SetConsoleCursorInfo(console,&ci);
}

void loadHighscore()
{
FILE *f=fopen("highscore.dat","r");
if(f){ fscanf(f,"%d",&highscore); fclose(f); }
}

void saveHighscore()
{
if(score>highscore)
{
FILE *f=fopen("highscore.dat","w");
fprintf(f,"%d",score);
fclose(f);
}
}

void initEnemies()
{
for(int i=0;i<MAX_ENEMIES;i++)
{
enemies[i].x=WIDTH+rand()%60;
enemies[i].y=HEIGHT-6;
enemies[i].speed=1+level/2;
enemies[i].type=rand()%2;
}
}

void initBullets()
{
for(int i=0;i<MAX_BULLETS;i++)
bullets[i].active=0;

for(int i=0;i<MAX_BOSS_BULLETS;i++)
bossBullets[i].active=0;
}

void shoot()
{
if(shootTimer>0) return;

for(int i=0;i<MAX_BULLETS;i++)
if(!bullets[i].active)
{
bullets[i].active=1;
bullets[i].x=dinoX+5;
bullets[i].y=dinoY+1;
shootTimer=SHOOT_COOLDOWN;
break;
}
}

void bossShoot()
{
if(rand()%25!=0) return;

for(int i=0;i<MAX_BOSS_BULLETS;i++)
if(!bossBullets[i].active)
{
bossBullets[i].y=bossY+1; // center row of 3-line bullet
break;
}
}

void physics()
{
velocity += gravity;
if(velocity > 5) velocity = 5;

dinoY += velocity;

if(dinoY>HEIGHT-6)
{
dinoY=HEIGHT-6;
velocity=0;
}
}

void updateEnemies()
{
for(int i=0;i<MAX_ENEMIES;i++)
{
enemies[i].x -= enemies[i].speed;

if(enemies[i].x<0)
enemies[i].x = WIDTH + rand()%60;

if(abs(enemies[i].x-dinoX)<2 &&
   abs(enemies[i].y-dinoY)<2)
{
health -= 10;

if(health<=0)
{
gameOver=1;
showGameOver();
}

enemies[i].x=WIDTH+rand()%60;
}
}
}

void updateBullets()
{
for(int i=0;i<MAX_BULLETS;i++)
if(bullets[i].active)
{
bullets[i].x += 2;

if(bullets[i].x>WIDTH)
bullets[i].active=0;

for(int j=0;j<MAX_ENEMIES;j++)
if(abs(bullets[i].x-enemies[j].x)<2 &&
   abs(bullets[i].y-enemies[j].y)<2)
{
score+=20;
enemies[j].x=WIDTH+rand()%80;
bullets[i].active=0;
}

if(abs(bullets[i].x-bossX)<6 &&
   abs(bullets[i].y-bossY)<3)
{
bossHP-=10;
bullets[i].active=0;
score+=50;
}
}

if(shootTimer>0) shootTimer--;
}

// ----------------------------------------------------------------
// Boss bullets are now 3 lines tall.
// Stored y is the TOP row; the bullet occupies y, y+1, y+2.
// Collision checks the centre row (y+1) for fairness.
// ----------------------------------------------------------------
void updateBossBullets()
{
for(int i=0;i<MAX_BOSS_BULLETS;i++)
if(bossBullets[i].active)
{
bossBullets[i].x--;

if(bossBullets[i].x<0)
bossBullets[i].active=0;

// Collision: use centre row (y+1) of the 3-line bullet
int cx = bossBullets[i].x;
int cy = bossBullets[i].y + 1;

if(abs(cx-dinoX)<2 &&
   abs(cy-dinoY)<2)
{
health-=10;
bossBullets[i].active=0;

if(health<=0)
{
gameOver=1;
showGameOver();
}
}
}
}

// ----------------------------------------------------------------
// Time-based passive scoring.
// Called every frame in both gameLoop and bossFight.
// Awards SCORE_PER_SECOND_BASE * level points per second.
// ----------------------------------------------------------------
void updatePassiveScore()
{
float pointsPerFrame = (float)(SCORE_PER_SECOND_BASE * level) / gameSpeed;
scoreAccum += pointsPerFrame;
if(scoreAccum >= 1.0f)
{
int pts = (int)scoreAccum;
score += pts;
scoreAccum -= pts;
}
}

void updateTerrain()
{
terrainOffset++;
bg1++;
bg2+=2;
bg3+=3;
}

void drawParallax()
{
for(int x=0;x<WIDTH;x++)
{
screen[(x+bg1)%6][x]='.';
screen[(x+bg2)%10][x]='*';
screen[(x+bg3)%14][x]='+';
}
}

void drawGround()
{
for(int x=0;x<WIDTH;x++)
{
int p=(x+terrainOffset)%6;
screen[HEIGHT-3][x]= (p<3)?'_':'-';
}
}

void drawGame()
{
clearBuffer();

drawParallax();
drawGround();

drawSprite(dino,4,dinoX,dinoY);

for(int i=0;i<MAX_ENEMIES;i++)
{
if(enemies[i].type==0)
drawSprite(enemy1,3,enemies[i].x,enemies[i].y);
else
drawSprite(enemy2,3,enemies[i].x,enemies[i].y);
}

for(int i=0;i<MAX_BULLETS;i++)
if(bullets[i].active)
drawText(bullets[i].x,bullets[i].y,">>");

render();

// Show time remaining in level
int framesLeft = (SECONDS_PER_LEVEL * gameSpeed) - frameCount;
int secsLeft   = framesLeft / gameSpeed;
if(secsLeft < 0) secsLeft = 0;

printf("LEVEL:%d SCORE:%d HEALTH:%d HIGHSCORE:%d NEXT LVL:%ds\n",
level,score,health,highscore,secsLeft);
}

void showGameOver()
{
system("cls");

printf("\nGAME OVER\n");
printf("Score: %d\n",score);
printf("Highscore: %d\n",highscore);

saveHighscore();

printf("\nPress any key...");
getch();

exit(0);
}

void bossFight()
{
    initBullets();

    bossHP = 200;
    bossX = 65;
    bossY = 6;

    int moveTimer = 0;

    while(bossHP > 0)
    {
        clearBuffer();

        drawParallax();
        drawGround();

        drawSprite(dino,4,dinoX,dinoY);

        // -------- BOSS MOVEMENT --------
        moveTimer++;

        if(moveTimer % 20 == 0)
        {
            if(dinoY < bossY) bossY--;
            else if(dinoY > bossY) bossY++;
        }

        // -------- DRAW BOSS --------
        drawText(bossX, bossY,     "  /MMMM\\  ");
        drawText(bossX, bossY + 1, " | O  O | ");
        drawText(bossX, bossY + 2, " | ---- | ");
        drawText(bossX, bossY + 3, "  \\____/  ");

        // -------- BOSS SHOOT --------
        // Spawn bullet; y is the TOP row of the 3-line bullet.
        // Keep it 1 row inside the boss rect so it looks natural.
        if(rand()%15 == 0)
        {
            for(int i=0;i<MAX_BOSS_BULLETS;i++)
            if(!bossBullets[i].active)
            {
                bossBullets[i].active = 1;
                bossBullets[i].x = bossX - 1;
                // Clamp so all 3 rows (y, y+1, y+2) stay on screen
                int spawnY = bossY + 1;
                if(spawnY + 2 >= HEIGHT - 3) spawnY = HEIGHT - 6;
                bossBullets[i].y = spawnY;
                break;
            }
        }

        updateBossBullets();

        // -------- DRAW boss bullets (3 lines tall) --------
        for(int i=0;i<MAX_BOSS_BULLETS;i++)
        if(bossBullets[i].active)
        {
            int bx = bossBullets[i].x;
            int by = bossBullets[i].y;
            drawText(bx, by,     "<=");  // top row
            drawText(bx, by + 1, "<<");  // middle row (thickest / centre)
            drawText(bx, by + 2, "<=");  // bottom row
        }

        // -------- PLAYER BULLETS (3 lines tall in boss fight) --------
        for(int i=0;i<MAX_BULLETS;i++)
        if(bullets[i].active)
        {
            int px = bullets[i].x;
            int py = bullets[i].y;
            drawText(px, py - 1, "=>");  // top row
            drawText(px, py,     ">>");  // middle row (spawn row, collision)
            drawText(px, py + 1, "=>");  // bottom row

            if(px >= bossX-1 &&
               px <= bossX + 12 &&
               py >= bossY-1 &&
               py <= bossY + 4)
            {
                bossHP -= 10;
                bullets[i].active = 0;
                score += 50;
            }
        }

        // -------- PASSIVE SCORE (continues during boss fight) --------
        updatePassiveScore();

        render();

        // -------- UI --------
        printf("BOSS HP: [");
        int bar = bossHP / 10;
        for(int i=0;i<20;i++)
            printf(i < bar ? "#" : " ");
        printf("] %d\n", bossHP);

        printf("PLAYER HP: %d  SCORE: %d\n", health, score);

        // -------- INPUT --------
        if(kbhit())
        {
            char ch=getch();

            if(ch=='j' && velocity==0) velocity=-7;
            if(ch=='s') shoot();
        }

        // -------- UPDATE --------
        physics();
        updateTerrain();
        updateBullets();

        Sleep(1000/gameSpeed);
    }

    printf("\nBOSS DEFEATED!\n");
    Sleep(2000);
}

// ----------------------------------------------------------------
// gameLoop — now time-based leveling.
// Each level lasts SECONDS_PER_LEVEL seconds (measured in frames).
// Enemy speed and game speed still scale with level.
// ----------------------------------------------------------------
void gameLoop()
{
while(level<=5)
{
setLevelColor();
levelDialogue();

initEnemies();
initBullets();

frameCount = 0;
scoreAccum = 0.0f;
gameSpeed  = BASE_FPS + (level * 1);

int levelFrames = SECONDS_PER_LEVEL * gameSpeed;

while(frameCount < levelFrames)
{
if(kbhit())
{
char ch=getch();
if(ch=='j' && velocity==0) velocity=-7;
if(ch=='s') shoot();
}

physics();
updateEnemies();
updateBullets();
updateTerrain();
updatePassiveScore();

frameCount++;

drawGame();

Sleep(1000/gameSpeed);
}

level++;
health+=20;

// Re-init enemy speeds for new level
initEnemies();
}

bossFight();
}

void menu()
{
while(1)
{
system("cls");

printf("==== ULTRA DINO ADVENTURE ====\n");
printf("1. Start Game\n");
printf("2. Exit\n");

char c=getch();

if(c=='1') break;
if(c=='2') exit(0);
}
}

int main()
{
console=GetStdHandle(STD_OUTPUT_HANDLE);

srand(time(0));
hideCursor();
loadHighscore();

menu();
gameLoop();

saveHighscore();

printf("\nFINAL SCORE: %d\n",score);

return 0;
}