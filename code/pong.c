#include "raylib.h"
#include "raymath.h" // needed for vector math
#include "config.h"
#include "pong.h"

GameState InitGameState(void)
{
    GameState state = {
        0, // scoreL
        0, // scoreR
        { // paddleL
            { // position x, y
                PADDLE_WIDTH * 1.5,
                RENDER_HEIGHT / 2,
            },
            PADDLE_SPEED,  // speed
            PADDLE_LENGTH, // length
            PADDLE_WIDTH,  // width
        },

        { // paddleR
            { // position x, y
                RENDER_WIDTH - PADDLE_WIDTH * 2.5,
                RENDER_HEIGHT / 2,
            },
            PADDLE_SPEED,  // speed
            PADDLE_LENGTH, // length
            PADDLE_WIDTH,  // width
        },

        { // ball
            { // position x, y
                RENDER_WIDTH / 2 + BALL_SIZE / 2,
                RENDER_HEIGHT / 2 + BALL_SIZE / 2,
            },
            { // speed x, y
                BALL_SPEED, 0,
            },
            BALL_SPEED, // maxSpeed
            BALL_SIZE, // size
        },
    };
    return state;
}

bool CheckCollisionBallPaddle(Ball ball, Paddle paddle)
{
    bool collision = false;

    if ((ball.position.x < (paddle.position.x + paddle.width) &&
         (ball.position.x + ball.size) > paddle.position.x) &&
        (ball.position.y < (paddle.position.y + paddle.length) &&
         (ball.position.y + ball.size) > paddle.position.y))
        collision = true;

    return collision;
}

void EdgeCollisionPaddle(Paddle *paddle)
{
    if (paddle->position.y <= 0)
        paddle->position.y = 0; // Don't get stuck
    if (paddle->position.y + paddle->length > RENDER_HEIGHT)
        paddle->position.y = RENDER_HEIGHT - paddle->length;
}

void BounceBallEdge(Ball *ball)
{
    int leftEdgeCollide = (ball->position.x <= 0);
    int rightEdgeCollide = (ball->position.x + ball->size >= RENDER_WIDTH);
    int topEdgeCollide = (ball->position.y <= 0);
    int bottomEdgeCollide = (ball->position.y + ball->size >= RENDER_HEIGHT);

    if (leftEdgeCollide && ball->speed.x < 0)
    {
        ball->speed.x *= -1; // Bounce
        ball->position.x = 0; // Don't get stuck
    }
    if (rightEdgeCollide && ball->speed.x > 0)
    {
        ball->speed.x *= -1;
        ball->position.x = RENDER_WIDTH - ball->size;
    }
    if (topEdgeCollide && ball->speed.y < 0)
    {
        ball->speed.y *= -1;
        ball->position.y = 0;
    }
    if (bottomEdgeCollide && ball->speed.y > 0)
    {
        ball->speed.y *= -1;
        ball->position.y = RENDER_HEIGHT - ball->size;
    }
}

void BounceBallPaddle(Ball *ball, Paddle *paddle)
{
    if (CheckCollisionBallPaddle(*ball, *paddle) == false)
        return;

    int paddleCenter = paddle->position.y + paddle->length / 2;
    int ballCenter = ball->position.y + ball->size / 2;
    float hitPosition =
        (float)(ballCenter - paddleCenter) / (paddle->length / 2);

    // Bounce off paddle
    if (paddle->position.x < ball->position.x)
    {
        ball->position.x = paddle->position.x + paddle->width + 1;
        ball->speed.x = fabs(ball->speed.x);
    }
    else
    {
        ball->position.x = paddle->position.x - ball->size - 1;
        ball->speed.x = -fabs(ball->speed.x);
    }

    // Increase ball speed
    float bounceMultiplier = 1.1;
    ball->maxSpeed *= bounceMultiplier;

    // Calculate the new angle based on hit position
    float maxAngle = 40.0f;
    float angleInDegrees = hitPosition * maxAngle;
    float angleInRadians = angleInDegrees * PI / 180.0f;

    // Set the new velocity components based on the angle
    float speed = ball->maxSpeed;
    ball->speed.x = cosf(angleInRadians) * speed;
    ball->speed.y = sinf(angleInRadians) * speed;

    // Ensure correct horizontal direction
    ball->speed.x = (ball->speed.x > 0 && paddle->position.x >= ball->position.x) ?
        -ball->speed.x : ball->speed.x;
    ball->speed.x = (ball->speed.x < 0 && paddle->position.x < ball->position.x) ?
        -ball->speed.x : ball->speed.x;

    // Change angle based on the velocity of the paddle
    ball->speed.y += (paddle->speed * GetFrameTime()) * 150;

    // Normalize speed
    ball->speed = Vector2Scale(Vector2Normalize(ball->speed),
                               Vector2Length(ball->speed));

    // Debug
    // TraceLog(LOG_INFO, "cpu paddle speed: %f\n", paddle->speed);
}

void UpdatePaddlePlayer1Input(Paddle *paddle)
{
    float newSpeed = 0.0f; // Not moving by default

    // W/S to move paddle
    if (IsKeyDown(KEY_W))
        newSpeed = -PADDLE_SPEED;
    if (IsKeyDown(KEY_S))
        newSpeed = PADDLE_SPEED;

    // Left Shift to speed up
    if (IsKeyDown(KEY_LEFT_SHIFT))
        newSpeed *= 2;

    // Update paddle
    paddle->speed = newSpeed;
    paddle->position.y += paddle->speed * GetFrameTime();
}

void UpdatePaddlePlayer2Input(Paddle *paddle)
{
    float newSpeed = 0.0f; // Not moving by default

    // O/L or Up/Down to move paddle
    if (IsKeyDown(KEY_O) || IsKeyDown(KEY_UP))
        newSpeed = -PADDLE_SPEED;
    if (IsKeyDown(KEY_L) || IsKeyDown(KEY_DOWN))
        newSpeed = PADDLE_SPEED;

    // Right Shift to speed up
    if (IsKeyDown(KEY_RIGHT_SHIFT))
        newSpeed *= 2;

    // Update paddle
    paddle->speed = newSpeed;
    paddle->position.y += paddle->speed * GetFrameTime();
}

void UpdatePaddleComputerPlayer(Paddle *paddle, Ball *ball)
{
    float newSpeed = 0.0f; // Not moving by default

    // Follow the ball
    if ((paddle->position.y + paddle->length / 10) > ball->position.y + ball->size)
        newSpeed = -PADDLE_SPEED * 2;
    if ((paddle->position.y + paddle->length - paddle-> length / 10) < ball->position.y)
        newSpeed = PADDLE_SPEED * 2;

    // Update Paddle
    paddle->speed = newSpeed;
    paddle->position.y += paddle->speed * GetFrameTime();

    // perfect AI
    // paddle->position.y = ball->position.y;

    // TODO: a seek position which periodically updates
}

void UpdateBall(Ball *ball)
{
    Vector2 deltaTimeSpeed = Vector2Scale(ball->speed, GetFrameTime());
    ball->position = Vector2Add(ball->position, deltaTimeSpeed);

    // Make sure diagonal movement isn't too steep
    float minimumXSpeed = 300.0f;
    if (fabsf(ball->speed.x) < minimumXSpeed) {
        ball->speed.x = (ball->speed.x >= 0) ?
            minimumXSpeed : -minimumXSpeed;
    }

    // Normalize speed
    ball->speed = Vector2Scale(Vector2Normalize(ball->speed), ball->maxSpeed);
}

void ResetBall(Ball *ball)
{
    Vector2 center = { RENDER_WIDTH / 2 + ball->size / 2, RENDER_HEIGHT / 2 + ball->size / 2 };
    ball->position = center;
    Vector2 direction = { GetRandomValue(-1,1), GetRandomValue(-BALL_SPEED, BALL_SPEED) };
    ball->speed = direction;
    ball->maxSpeed = BALL_SPEED;
}

void UpdatePong(GameState *pong)
{
    // Update position
    UpdatePaddlePlayer1Input(&pong->paddleL);
    UpdatePaddlePlayer2Input(&pong->paddleR);
    // UpdatePaddleComputerPlayer(&pong->paddleL, &pong->ball);
    // UpdatePaddleComputerPlayer(&pong->paddleR, &pong->ball);
    UpdateBall(&pong->ball);

    // Collision logic
    BounceBallEdge(&pong->ball);
    BounceBallPaddle(&pong->ball, &pong->paddleL);
    BounceBallPaddle(&pong->ball, &pong->paddleR);
    EdgeCollisionPaddle(&pong->paddleL);
    EdgeCollisionPaddle(&pong->paddleR);
}

void DrawScores(GameState *pong)
{
    int fontSize = 180;

    const char *scoreLmsg = TextFormat("%2i", pong->scoreL);
    int scoreLWidth = MeasureText(scoreLmsg, fontSize);
    int scoreLposX = (RENDER_WIDTH - scoreLWidth) / 4;

    const char *scoreRmsg = TextFormat("%2i", pong->scoreR);
    int scoreRWidth = MeasureText(scoreRmsg, fontSize);
    int scoreRposX = 3 * RENDER_WIDTH / 4 - scoreRWidth;

    int scorePosY = 50;
    DrawText(scoreLmsg, scoreLposX, scorePosY, fontSize, RAYWHITE);
    DrawText(scoreRmsg, scoreRposX, scorePosY, fontSize, RAYWHITE);
}

void DrawDottedLine(void)
{
    int dashHeight = 50;
    int gapHeight = 50;
    int lineWidth = 25;

    for (int y = 0; y < RENDER_HEIGHT; y += dashHeight + gapHeight) {
        DrawRectangle(RENDER_WIDTH / 2 - lineWidth / 2, y, lineWidth, dashHeight, WHITE);
    }
}

void DrawGame(GameState *pong)
{
    // Draw line down middle
    DrawDottedLine();

    // Draw score
    DrawScores(pong);

    // Draw paddles
    DrawRectangle(pong->paddleR.position.x, pong->paddleR.position.y,
                  pong->paddleR.width, pong->paddleR.length, WHITE);
    DrawRectangle(pong->paddleL.position.x, pong->paddleL.position.y,
                  pong->paddleL.width, pong->paddleL.length, WHITE);

    // Draw ball
    DrawRectangle(pong->ball.position.x, pong->ball.position.y,
                  pong->ball.size, pong->ball.size, WHITE);
}
