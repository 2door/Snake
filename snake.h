#ifndef SNAKE_H
#define SNAKE_H

#include "segment.h"

class Snake {
    Segment* head;
    Segment* tail;
    int x_limit;
    int y_limit;
    unsigned int length;
    unsigned int score;
    void AddSegment();

    public:
        Snake();
        Snake(int headX, int headY, int limit);
        void Delete();
        void SetDirection(Direction d);
        unsigned int GetLength();
        unsigned int GetScore();
        unsigned int EatPallet();
        unsigned int Move();
        unsigned int** GetSnakePosition();
};
#endif