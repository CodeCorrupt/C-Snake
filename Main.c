// I wrote this as my first major programming project in college. Freshman year (Fall 2011)

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include <time.h>
#include <math.h>
#include <string.h>

//Struct used for holding coords
struct coord
{
    int x;
    int y;
    int present;
    struct coord* next;
};

//"Game Master" mode: allows for controle over all aspects of the game, while in game (1 = true, 0 = false)
const int GM_MODE = 0;
const int NEVER_DIE = 1; // only works if GM_MODE is turned on

//Should the walls kill you? or teleport you? (1 = kill, 0 = teleport)
const int WALLS_KILL = 0;

//Direction const's !!!DON'T CHANGE NUMBERING!!!
//(changing numbers will allow snake to double back on itself)
const int UP = 0;
const int RIGHT = 1;
const int DOWN = 2;
const int LEFT = 3;

//Snake info
const int START_X = 0;
const int START_Y = 0;
const int START_DIRECTION = 2; ///////////Use the numbers from "Direction const's"
const int START_LENGTH = 5;
const int SNAKE_DX = 10;
const int SNAKE_DY = 10;
const int GROWTH_PER_FOOD = 5;

//Screen info
const int SCREEN_WIDTH = 49; ////////////In number of squares NOT pixels!
const int SCREEN_HEIGHT = 29;////////////In number of squares NOT pixels!
const int SCREEN_BPP = 32;

//Frame rate info
const int FRAMES_PER_SECOND = 60;
const int FRAMES_PER_MOVE = 5;

//Values for bkg color (must be between 0 and 255)
const int BKG_RED = 255;
const int BKG_GREEN = 255;
const int BKG_BLUE = 255;

//Values for key color (must be between 0 and 255)
const int KEY_RED = 255;
const int KEY_GREEN = 255;
const int KEY_BLUE = 255;

//TTF info
const int TEXT_RED = 0;
const int TEXT_GREEN = 0;
const int TEXT_BLUE = 0;
const int TEXT_SIZE = 28;

//Event flag constants
const int PLAYING = 0;
const int PAUSED = 1;
const int MENU = 2;
const int QUIT = 3;
const int GAMEOVER = 4;
const int NEWGAME = 5;

//used in calculating time bonus for score squrt(time_played/TIME_BONUS_DENOM)
const double TIME_BONUS_DENOM = 25;
const int CAPPED_BONUS = 1;

//Functions
struct coord* move(struct coord *front, int direction, int* game_state);
int init();
int load_files();
SDL_Surface *load_image(char filename[]);
int draw_snake_bite_tale(struct coord *front, int length, int* game_state);
int draw(int x, int y, SDL_Surface* source, SDL_Surface* destination);
void clean_up();
void get_key(int* direction, int* prev_direction, int* game_state, int* add_length, int* fpm);
void free_coord(struct coord *front);
void make_food(struct coord *front, struct coord *food);
void check_food(struct coord *front, struct coord *food, int* length);
void check_border(struct coord *front, int direction, int* game_state);
int get_score(int start_time, int length);
int check_high_score(int int_score);

//The surfaces
SDL_Surface *red = NULL;
SDL_Surface *blue = NULL;
SDL_Surface *green = NULL;
SDL_Surface *play = NULL;
SDL_Surface *paused = NULL;
SDL_Surface *screen = NULL;
SDL_Surface *score = NULL;
SDL_Surface *congrats = NULL;

//The font that's going to be used
TTF_Font *font = NULL;

//The event structure
SDL_Event event;

//The color of the font
SDL_Color text_color;

FILE* ofp_info;
FILE* fp_scores; //Only "fp" because the file is input and output


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* args[])
{
    //Set up game variables
    int game_state = MENU;
    int prev_state = GAMEOVER;
    int direction;
    int frame_ticks;
    int frame_count;
    int length;
    int prev_direction;
    int start_time;
    int int_score = -1;

    char your_score[25];

    text_color.r = TEXT_RED;
    text_color.g = TEXT_GREEN;
    text_color.b = TEXT_BLUE;

    int add_length; //--------------------------------------------------------------------------------------------------------------- GM_MODE
    int fpm = FRAMES_PER_MOVE; //--------------------------------------------------------------------------------------------------------------- GM_MODE

    //Initialize
    if(init() == 1)
    {
        return 1;
    }

    //Load the files
    if(load_files() == 1)
    {
        return 1;
    }

    //Create the snake "Linked List" start
    struct coord *head = NULL;
    struct coord *food = NULL;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Main loop/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    while(game_state != QUIT)
    {
        frame_ticks = SDL_GetTicks();

        get_key(&direction, &prev_direction, &game_state, &add_length, &fpm);

        if(game_state == MENU)
        {
            if (prev_state != MENU)
            {
                //Show mouse
                SDL_ShowCursor(SDL_ENABLE);

                //Draw play button on screen
                draw(((SCREEN_WIDTH*SNAKE_DX - paused->clip_rect.w)/2), ((SCREEN_HEIGHT*SNAKE_DY - paused->clip_rect.h)/2), play, screen);
            }
            prev_state = MENU;
        }

        if (game_state == NEWGAME)
        {
            //Show mouse
            SDL_ShowCursor(SDL_ENABLE);

            //Free the old snake in memory
            if (head != NULL)
                free_coord(head);
            if (food != NULL)
                free_coord(food);

            //Set event flags
            direction = START_DIRECTION;
            prev_direction = START_DIRECTION;
            length = START_LENGTH;
            add_length = 0;
            frame_count = 0;


            //Create new piece
            head = (struct coord*)malloc(sizeof(struct coord));
            food = (struct coord*)malloc(sizeof(struct coord));

            //Place snake in starting position.
            head->x = (START_X);
            head->y = (START_Y);
            head->present = -1;
            head->next = NULL;

            //Set up food for new game
            food->x = 0;
            food->y = 0;
            food->present = 0;
            food->next = NULL;

            start_time = SDL_GetTicks();

            //Start the game
            game_state = PLAYING;
        }

        if (game_state == PLAYING)
        {
            if (prev_state != PLAYING)
            {
                //Hide mouse
                SDL_ShowCursor(SDL_DISABLE);
            }

            //Fill the screen white
            SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, BKG_RED, BKG_GREEN, BKG_BLUE));

            //Check if snake has gone past the walls, if it has, respond according to WALLS_KILL
            check_border(head, direction, &game_state);

            //Move snake one square forward
            if (frame_count%(fpm) == 0) //--------------------------------------------------------------------------------------------------------------- GM_MODE && Reg mode
            {                                                                                                         //If reg mode, you cant change fpm after initialization
                head = move(head, direction, &game_state);
                frame_count = 0;

                prev_direction = direction;
            }

            //Make new food if food has been eaten
            if (food->present == 0)
                make_food(head, food);

            //Check if food has been eaten, and if eaten, add to length
            check_food(head, food, &length);

            if (GM_MODE == 1) //--------------------------------------------------------------------------------------------------------------- GM_MODE
            {
                if (add_length == 1)
                {
                    length = length + GROWTH_PER_FOOD;
                    add_length = 0;
                }
            }

            //Draw the food
            if (draw(food->x * SNAKE_DX, food->y * SNAKE_DY, green, screen) == 1)
            {
                return 1;
            }

            //convert position into pixles and then draw() snake
            if (draw_snake_bite_tale(head, length, &game_state) == 1)
            {
                return 1;
            }

            frame_count++;

            prev_state = PLAYING;
        }

        if (game_state == PAUSED)
        {
            if (prev_state != PAUSED)
            {
                //Show mouse
                SDL_ShowCursor(SDL_ENABLE);

                draw(((SCREEN_WIDTH*SNAKE_DX - paused->clip_rect.w)/2), ((SCREEN_HEIGHT*SNAKE_DY - paused->clip_rect.h)/2), paused, screen);
            }
            prev_state = PAUSED;
        }

        if (game_state == GAMEOVER)
        {
            if (prev_state != GAMEOVER)
            {
                //Show mouse
                SDL_ShowCursor(SDL_ENABLE);

                int_score = get_score(start_time, length);

                if (int_score != -1)
                {
                    sprintf(your_score, "Your score is: %d", int_score);

                    //Render the text
                    score = TTF_RenderText_Solid(font, your_score, text_color);

                    //If there was an error in rendering the text
                    if(score == NULL)
                    {
                        return 1;
                    }

                    //Apply score to the screen
                    draw(((SCREEN_WIDTH*SNAKE_DX - score->clip_rect.w)/2) , (SCREEN_HEIGHT*SNAKE_DY - score->clip_rect.h), score, screen);
                }

                if (check_high_score(int_score) && GM_MODE == 0)
                {
                    //Render the text
                    congrats = TTF_RenderText_Solid(font, "!>>HIGH SCORE<<!", text_color);

                    //If there was an error in rendering the text
                    if(score == NULL)
                    {
                        return 1;
                    }

                    //Apply score to the screen
                    draw(((SCREEN_WIDTH*SNAKE_DX - congrats->clip_rect.w)/2) , (SCREEN_HEIGHT*SNAKE_DY - congrats->clip_rect.h - score->clip_rect.h), congrats, screen);

                }
            }
            prev_state = GAMEOVER;
            game_state = MENU;
        }


        //Update the screen
        if(SDL_Flip(screen) == -1)
        {
            return 1;
        }

        //Wait for fps
        if((SDL_GetTicks() - frame_ticks) < 1000/FRAMES_PER_SECOND)
        {
            //Sleep the remaining frame time
            SDL_Delay((1000/FRAMES_PER_SECOND) - (SDL_GetTicks() - frame_ticks));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //Free the images and quit SDL
    clean_up();

    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// Precondition(s): Coord *front != NULL, direction = UP - LEFT,
// Postcondition(s): A new node is linked in to the link list for the snake
struct coord* move(struct coord *front, int direction, int* game_state)
{
    struct coord *head;

    //Create new piece
    head = (struct coord*)malloc(sizeof(struct coord));

    //Save the new piece's coordinates
    if (direction == UP)
    {
        head->x = (front->x);
        head->y = (front->y - 1);
    }
    else if (direction == RIGHT)
    {
        head->x = (front->x + 1);
        head->y = front->y;
    }
    else if (direction == DOWN)
    {
        head->x = (front->x);
        head->y = (front->y + 1);
    }
    else if (direction == LEFT)
    {
        head->x = (front->x - 1);
        head->y = (front->y);
    }

    //Link new node to front
    head->next = front;

    //retern new head's mem location
    return head;
}

// Precondition(s): None
// Postcondition(s): All SDL functions are initialized
int init()
{
    //Set random number
    srand(time(0));

    //Initialize all SDL subsystems
    if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
        return 1;
    }

    //Set up the screen
    screen = SDL_SetVideoMode(SCREEN_WIDTH*SNAKE_DX, SCREEN_HEIGHT*SNAKE_DY, SCREEN_BPP, SDL_SWSURFACE);

    //If there was an error in setting up the screen
    if(screen == NULL)
    {
        return 1;
    }

    //Initialize SDL_ttf
    if(TTF_Init() == -1)
    {
        return 1;
    }

    //Set the window caption
    SDL_WM_SetCaption("Tyler & Chris's >>> SNAKE! <<<", NULL);

    //Fill the screen white
    SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, BKG_RED, BKG_GREEN, BKG_BLUE));

    //If everything initialized fine
    return 0;
}

// Precondition(s): None
// Postcondition(s): Images & font are loaded
int load_files()
{
    //Load the images
    red = load_image("red_10.png");
    blue = load_image("blue_10.png");
    green = load_image("green_10.png");
    play = load_image("play.png");
    paused = load_image("paused.png");

    //Open the font
    font = TTF_OpenFont("cooper_black.ttf", TEXT_SIZE);

    //If there was an problem loading the sprite map
    if(red == NULL || blue == NULL || green == NULL || play == NULL || paused == NULL)
    {
        return 1;
    }

    //If there was an error in loading the font
    if( font == NULL )
    {
        return 1;
    }

    //If eveything loaded fine
    return 0;
}

// Precondition(s): "filename" is in the same folder as .exe
// Postcondition(s): Returns the pointer to the SDL_Surface of the image
SDL_Surface *load_image(char filename[])
{
    //Temporary storage for the image that's loaded
    SDL_Surface* loadedImage = NULL;

    //The optimized image that will be used
    SDL_Surface* optimizedImage = NULL;

    //Load the image
    loadedImage = IMG_Load(filename);

    //If nothing went wrong in loading the image
    if(loadedImage != NULL)
    {
        //Create an optimized image
        optimizedImage = SDL_DisplayFormat(loadedImage);

        //Free the old image
        SDL_FreeSurface(loadedImage);
    }

    //If the image was optimized just fine
    if(optimizedImage != NULL)
    {
        //Map the color key
        Uint32 colorkey = SDL_MapRGB(optimizedImage->format, KEY_RED, KEY_BLUE, KEY_GREEN);

        //Set all pixels of color _____ to be transparent
        SDL_SetColorKey(optimizedImage, SDL_SRCCOLORKEY, colorkey);
    }

    //Return the optimized image
    return optimizedImage;
}

// Precondition(s): Front != NULL, length > 0, images are loaded and screen in initialized
// Postcondition(s): Snake is drawn and collition with tail is checked.
int draw_snake_bite_tale(struct coord *front, int length, int* game_state)
{
    int x, y;
    struct coord *next;
    struct coord *prev;

    //Set the pointer to the next number to the variable "next"
    next = front;
    int i;
    for (i=0; i<length; i++)
    {
        if (next == NULL)
            break;

        prev = next;

        //Convert position into pixles
        x = (next->x * SNAKE_DX);
        y = (next->y * SNAKE_DY);

        if (draw(x, y, blue, screen) == 1)
        {
            return 1;
        }

        if (i != 0)
        {
            //See if the head of the snake crosses the current pice's path (you bit your tale)
            if (front->x == next->x && front->y == next->y)
            {
                if (GM_MODE == 0 || (NEVER_DIE == 0 && GM_MODE == 1)) //--------------------------------------------------------------------------------------------------------------- GM_MODE
                    *game_state = GAMEOVER;

                if (draw(x, y, red, screen) == 1)
                {
                    return 1;
                }
            }
        }

        next = next->next;

        //Free the last node in the list after it is drawn
        if (i == (length - 1))
        {
            prev->next = NULL;
            free(next);
        }
    }

    return 0;
}

// Precondition(s): Source and destination are loaded and initialized.
// Postcondition(s): Source is drawn overtop of destination
int draw(int x, int y, SDL_Surface* source, SDL_Surface* destination)
{
    //Make a temporary rectangle to hold the offsets
    SDL_Rect offset;

    //Give the offsets to the rectangle
    offset.x = x;
    offset.y = y;

    //Blit the surface
    SDL_BlitSurface(source, NULL, destination, &offset);

    return 0;
}

// Precondition(s): None
// Postcondition(s): Frees memory of images and texts and closes SDL and TTF
void clean_up()
{
    //Free the surfaces
    SDL_FreeSurface(red);
    SDL_FreeSurface(blue);
    SDL_FreeSurface(green);
    SDL_FreeSurface(play);
    SDL_FreeSurface(paused);
    SDL_FreeSurface(score);
    SDL_FreeSurface(congrats);

    //Close the font that was used
    TTF_CloseFont(font);

    //Quit SDL_ttf
    TTF_Quit();

    //Quit SDL
    SDL_Quit();
}

// Precondition(s): None
// Postcondition(s): Responds to key presses
void get_key(int* direction, int* prev_direction, int* game_state, int* add_length, int* fpm)
{
    //While there's events to handle
    while(SDL_PollEvent(&event))
    {
        //If the user has Xed out the window
        if(event.type == SDL_QUIT)
        {
            //Quit the program
            *game_state = QUIT;
        }

        if (event.type == SDL_KEYDOWN)
        {
            //Respond to which arrow was pressed
            switch( event.key.keysym.sym )
            {
                //Direction keys
                case SDLK_UP: *direction = UP; break;
                case SDLK_DOWN: *direction = DOWN; break;
                case SDLK_LEFT: *direction = LEFT; break;
                case SDLK_RIGHT: *direction = RIGHT; break;

                //'P' = pause game
                case SDLK_p:
                    if (*game_state == PAUSED)
                    {
                        *game_state = PLAYING;
                    }
                    else if (*game_state == PLAYING)
                    {
                        *game_state = PAUSED;
                    }
                break;

                //'r' = Restart
                case SDLK_r:
                    if (*game_state == PLAYING)
                    {
                        *game_state = GAMEOVER;
                    }
                break;

                //'RETURN = start game from menu
                case SDLK_RETURN:
                    if (*game_state == MENU)
                    {
                        *game_state = NEWGAME;
                    }
                break;

                //Esc key = QUIT
                case SDLK_ESCAPE: *game_state = QUIT; break;

                //'L' key acts like you hit a piece of food if in GM_MODE
                case SDLK_l: //--------------------------------------------------------------------------------------------------------------- GM_MODE
                    if (GM_MODE == 1)
                        *add_length = 1;
                break;

                //'PAGEUP' Speeds up the snake if in GM_MODE
                case SDLK_PAGEUP: //--------------------------------------------------------------------------------------------------------------- GM_MODE
                    if (GM_MODE == 1)
                    {
                        if (*fpm > 1)
                            *fpm = *fpm - 1;
                    }
                break;

                //'PAGEDOWN' Slows down the snake if in GM_MODE
                case SDLK_PAGEDOWN: //--------------------------------------------------------------------------------------------------------------- GM_MODE
                    if (GM_MODE == 1)
                        *fpm = *fpm + 1;
                break;


                //Ignore everything else
                default: break;
            }
        }
    }

    if (*direction%2 == *prev_direction%2)
    {
        *direction = *prev_direction;
    }
}

// Precondition(s): None
// Postcondition(s): Frees the link list from "front" till the end (piece->next == NULL)
void free_coord(struct coord *front)
{
    struct coord *next;
    struct coord *piece;

    //Set the current piece were working with to front
    piece = front;

    //While not at the end of the list, Free that piece, move to the next and repeat
    while (piece != NULL)
    {
        next = piece->next;
        free(piece);
        piece = next;
    }

}

// Precondition(s): None
// Postcondition(s): Randomly creates a piece of food that is not overtop of the snake
void make_food(struct coord *front, struct coord *food)
{

    int good_food = 0;

    struct coord *piece;

    while (good_food == 0)
    {
        //Randomly create the food coord
        food->x = rand()%SCREEN_WIDTH;
        food->y = rand()%SCREEN_HEIGHT;

        //Assume good is good untill proven bad
        good_food = 1;

        piece = front;

        while (piece != NULL)
        {
            //If food is ontop of any part of the snake, good_food = BAD!
            if ((food->x == piece->x) && (food->y == piece->y))
            {
                good_food = 0;
            }

            piece = piece->next;
        }
    }

    food->present = 1;
}

// Precondition(s): Front != NULL, food != NULL
// Postcondition(s): Check if the head of the snake intersects the food, if it does add to length
void check_food(struct coord *front, struct coord *food, int* length)
{
    if ((front->x == food->x) && (front->y == food->y))
    {
        *length = *length + GROWTH_PER_FOOD;
        food->present = 0;
    }
}

// Precondition(s): front != NULL
// Postcondition(s): depending on WALLS_KILL either ends game if snake goes outside, or moves snake to other side
void check_border(struct coord *front, int direction, int* game_state)
{
    //Check to see if head of snake went outside screen
    if (WALLS_KILL == 0)                            //If walls DO NOT kill then move snake through the wall to other side
    {
        if (front->x  < 0)
        {
            front->x = (SCREEN_WIDTH - 1);
        }

        if (front->y >= (SCREEN_HEIGHT))
        {
            front->y = (0);
        }

        if (front->x >= (SCREEN_WIDTH))
        {
            front->x = (0);
        }

        if (front->y < 0)
        {
            front->y = (SCREEN_HEIGHT - 1);
        }
    }                                               //If walls DO kill then change game_state
    else if ((front->x < 0) || (front->y < 0) || (front->y >= (SCREEN_HEIGHT)) || (front->x >= (SCREEN_WIDTH)))
        *game_state = GAMEOVER;
}

// Precondition(s): None
// Postcondition(s): Returns your calculated score
int get_score(int start_time, int length)
{
    //Open the output file for the info
    ofp_info = fopen("game_info.txt", "a");

    int time_played = SDL_GetTicks() - start_time;

    //Set the time bonus bassed on the time you spent playing.
    double time_bonus = sqrt(time_played/TIME_BONUS_DENOM);

    //Cap the time bonus if CAPPED_BONUS == 1
    if (time_bonus > length && CAPPED_BONUS == 1)
        time_bonus = length;

    //Only enter Game info if GM_MODE is off
    if (GM_MODE == 0)
    {
        fprintf(ofp_info, "Game info:\n");
        fprintf(ofp_info, "    Time played (seconds) = %d:%d:%.3lf\n",((time_played/1000)/60)/60 , ((time_played/1000)/60)%60, (time_played/1000)%60 + (time_played%1000)/1000.0);
        fprintf(ofp_info, "    Length                = %d\n", length);
        fprintf(ofp_info, "    Time Bonus            = %d\n", (int)time_bonus);
        fprintf(ofp_info, "    >>> Score (capped time bonus)+length = >> %d <<\n\n", (length + (int)time_bonus));
    }

    //Close the output file
    fclose(ofp_info);

    return (length + (int)time_bonus);
}

// Precondition(s): None
// Postcondition(s): Checks for high score, and appends the current score to the end of the file.
int check_high_score(int int_score)
{
    //Assume you have a high score till you find a score larger in the history
    int high_score = 1;
    int compare = 0;

    //Only check for high scores if your not in GM_MODE
    if (GM_MODE == 0)
    {
        //Open the file holding the scores to read and check for highest score
        fp_scores = fopen("scores.txt", "r");

        while (fscanf(fp_scores, "%d", &compare) != EOF)
        {
            if (compare >= int_score)
                high_score = 0;
        }

        //Close the output file
        fclose(fp_scores);

        //Open the scores file so we can add the new score
        fp_scores = fopen("scores.txt", "a");

        fprintf(fp_scores, "%d\n", int_score);

        //Close the file
        fclose(fp_scores);
    }

    return high_score;
}






