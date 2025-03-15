#include "raylib.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath> // For distance calculation
#include <ctime> // For seeding random number generator

// Game constants
const int screenWidth = 1280;
const int screenHeight = 720;
const float playerSpeed = 5.0f;
const float gravity = 0.5f;
const float jumpForce = -12.0f;
const int tileSize = 64;  // Size of each tile in pixels
const float batScale = 0.1875f; // Bat scale factor
const float batSpeed = 2.0f; // Bat movement speed
const int invulnerabilityFrames = 60; // 1 second at 60 FPS

// Player structure
struct Player {
    Vector2 position;
    Vector2 velocity;
    float speed;
    bool isJumping;
    Rectangle bounds;
    bool isFacingRight;  // To track player direction for sprite flipping
    Texture2D texture;
    int invulnerabilityTimer; // Timer for invulnerability after collision
};

// Platform/ground structure
struct Platform {
    Rectangle rect;
    Texture2D texture;
};

// Flag structure
struct Flag {
    Vector2 position;
    Rectangle bounds;
    Texture2D texture;
    bool reached;
    float waveTimer;
    Color color;
};

// Bat enemy structure
struct Bat {
    Vector2 position;
    Vector2 velocity;
    Rectangle bounds;
    Texture2D texture;
    float scale;
    bool active;
    int respawnTimer; // Timer to prevent immediate respawn
    
    // Animation properties
    Texture2D frames[8]; // Array to hold animation frames
    int currentFrame;    // Current animation frame
    int frameCounter;    // Counter for animation timing
    int framesSpeed;     // Speed of animation (frames per update)
    
    // Method to calculate the scaled width
    float GetScaledWidth() const {
        return texture.width * scale;
    }
    
    // Method to calculate the scaled height
    float GetScaledHeight() const {
        return texture.height * scale;
    }
};

// Score structure
struct Score {
    int bat;    // Points for the bat
    int player; // Points for the player
};

// Function to calculate distance between two points
float CalculateDistance(Vector2 p1, Vector2 p2) {
    return sqrtf(powf(p2.x - p1.x, 2) + powf(p2.y - p1.y, 2)); // Using sqrtf and powf for float precision
}

// Function to respawn bat at a random position away from player
void RespawnBat(Bat& bat, const Player& player) {
    // Calculate minimum distance (5 bat lengths)
    float minDistance = bat.GetScaledWidth() * 5;
    
    // Keep generating positions until we find one that's far enough
    Vector2 newPos;
    float distance;
    int attempts = 0;
    const int maxAttempts = 100; // Prevent infinite loop
    
    do {
        // Generate random position in the sky
        newPos.x = static_cast<float>(GetRandomValue(0, static_cast<int>(screenWidth - bat.GetScaledWidth())));
        newPos.y = static_cast<float>(GetRandomValue(0, screenHeight / 3)); // Top third of the screen
        
        // Calculate distance to player
        distance = CalculateDistance(newPos, player.position);
        
        attempts++;
        if (attempts >= maxAttempts) {
            // If we can't find a good position after many attempts, 
            // place the bat at the opposite side of the screen from the player
            newPos.x = (player.position.x < screenWidth / 2) ? 
                       (screenWidth - bat.GetScaledWidth() - 10) : 10;
            newPos.y = 50;
            break;
        }
    } while (distance < minDistance);
    
    // Set the new position
    bat.position = newPos;
    
    // Reset velocity (optional: give it a random direction)
    // Make sure the bat has a non-zero velocity
    bat.velocity.x = (GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f) * batSpeed;
    bat.velocity.y = static_cast<float>(GetRandomValue(0, 1)) * batSpeed;
    
    // If both velocities are zero, give it a default movement
    if (bat.velocity.x == 0 && bat.velocity.y == 0) {
        bat.velocity.x = batSpeed;
    }
    
    // Update bounds
    bat.bounds.x = bat.position.x;
    bat.bounds.y = bat.position.y;
    bat.bounds.width = bat.GetScaledWidth();
    bat.bounds.height = bat.GetScaledHeight();
    
    // Activate the bat
    bat.active = true;
}

// Function to load a texture or create a fallback if file doesn't exist
Texture2D LoadTextureOrDefault(const std::string& filename, int width, int height, Color color) {
    // Print debug info
    std::cout << "Attempting to load texture: " << filename << std::endl;
    
    // Check if file exists
    if (FileExists(filename.c_str())) {
        std::cout << "File exists! Loading texture from file." << std::endl;
        Texture2D texture = LoadTexture(filename.c_str());
        std::cout << "Texture loaded. Width: " << texture.width << ", Height: " << texture.height << std::endl;
        return texture;
    } else {
        std::cout << "File not found. Creating fallback texture." << std::endl;
        // Create a placeholder texture if the file doesn't exist
        Image img = GenImageColor(width, height, color);
        Texture2D texture = LoadTextureFromImage(img);
        UnloadImage(img);
        return texture;
    }
}

// Function to generate random stair positions and return the top stair position
Vector2 GenerateRandomStairs(std::vector<Platform>& platforms, Texture2D groundTexture) {
    // Random parameters for stairs
    int stairCount = GetRandomValue(4, 8); // Random number of stairs between 4 and 8
    int startX = GetRandomValue(screenWidth / 2, screenWidth - 300); // Random starting X position
    int startY = GetRandomValue(screenHeight - 300, screenHeight - 100); // Random starting Y position
    int stairWidth = GetRandomValue(1, 3) * tileSize; // Random stair width (1-3 tiles)
    
    // Create the staircase
    Vector2 topStairPos = {0, 0}; // Will store the position of the top stair
    
    for (int i = 0; i < stairCount; i++) {
        Platform stair = {
            {startX + (i * tileSize / 2), startY - (i * tileSize), stairWidth, tileSize},
            groundTexture
        };
        platforms.push_back(stair);
        
        // Save the position of the top stair
        if (i == stairCount - 1) {
            topStairPos.x = stair.rect.x + stair.rect.width - 40; // Position flag near the right edge of top stair
            topStairPos.y = stair.rect.y - 60; // Position flag above the top stair
        }
    }
    
    return topStairPos;
}

int main() {
    // Initialize audio device first
    InitAudioDevice();
    
    // Seed random number generator
    SetRandomSeed((unsigned int)time(NULL));
    
    // Initialize window
    InitWindow(screenWidth, screenHeight, "Side Scroller Game");
    SetTargetFPS(60);
    
    // Get the current working directory and print it
    const char* currentDir = GetWorkingDirectory();
    std::cout << "Current working directory: " << currentDir << std::endl;
    
    // Define the asset paths - use absolute paths to be sure
    std::string assetPath = std::string(currentDir) + "/assets/";
    // Try different paths to find the assets
    std::string playerImagePath = assetPath + "player1.png"; // Changed to player1.png
    std::string groundImagePath = assetPath + "ground.png";
    std::string backgroundImagePath = assetPath + "background.png";
    std::string batBasePath = assetPath + "02-Fly/__Bat02_Fly_00";
    std::string flagImagePath = assetPath + "flag.png";
    std::string musicPath = assetPath + "game-music-loop.mp3";
    
    // Also try looking in the parent directory
    if (!FileExists(playerImagePath.c_str())) {
        playerImagePath = "../assets/player1.png";
    }
    if (!FileExists(groundImagePath.c_str())) {
        groundImagePath = "../assets/ground.png";
    }
    if (!FileExists(backgroundImagePath.c_str())) {
        backgroundImagePath = "../assets/background.png";
    }
    if (!FileExists(flagImagePath.c_str())) {
        flagImagePath = "../assets/flag.png";
    }
    if (!FileExists(musicPath.c_str())) {
        musicPath = "../assets/game-music-loop.mp3";
    }
    
    // Check if bat animation frames exist in parent directory
    bool batFramesInParent = false;
    std::string testBatFrame = assetPath + "02-Fly/__Bat02_Fly_000.png";
    if (!FileExists(testBatFrame.c_str())) {
        batBasePath = "../assets/02-Fly/__Bat02_Fly_00";
        testBatFrame = "../assets/02-Fly/__Bat02_Fly_000.png";
        if (FileExists(testBatFrame.c_str())) {
            batFramesInParent = true;
        }
    }
    
    std::cout << "Player image path: " << playerImagePath << std::endl;
    std::cout << "Bat frames base path: " << batBasePath << std::endl;
    
    // Load textures with fallbacks if files don't exist
    Texture2D playerTexture = LoadTextureOrDefault(playerImagePath, 40, 40, BLUE);
    Texture2D groundTexture = LoadTextureOrDefault(groundImagePath, tileSize, tileSize, DARKGRAY);
    Texture2D flagTexture = LoadTextureOrDefault(flagImagePath, 40, 60, YELLOW);
    
    // Load bat animation frames
    Texture2D batFrames[8];
    bool usingBatFallback = true;
    
    // Try to load all bat animation frames
    for (int i = 0; i < 8; i++) {
        std::string frameFilename = batBasePath + std::to_string(i) + ".png";
        std::cout << "Attempting to load bat frame: " << frameFilename << std::endl;
        
        if (FileExists(frameFilename.c_str())) {
            batFrames[i] = LoadTexture(frameFilename.c_str());
            usingBatFallback = false;
            std::cout << "Loaded bat frame " << i << " successfully" << std::endl;
        } else {
            // Create a fallback texture for this frame
            Image img = GenImageColor(60, 40, RED);
            batFrames[i] = LoadTextureFromImage(img);
            UnloadImage(img);
            std::cout << "Using fallback for bat frame " << i << std::endl;
        }
    }
    
    // Use the first frame as the main texture
    Texture2D batTexture = batFrames[0];
    
    // For background, try to load from file or create a gradient
    Texture2D backgroundTexture;
    if (FileExists(backgroundImagePath.c_str())) {
        backgroundTexture = LoadTexture(backgroundImagePath.c_str());
    } else {
        // Create a gradient background manually
        Image backgroundImg = GenImageColor(screenWidth, screenHeight, SKYBLUE);
        // Draw a gradient effect manually
        for (int y = 0; y < screenHeight; y++) {
            float factor = static_cast<float>(y) / screenHeight;
            Color color = {
                static_cast<unsigned char>(SKYBLUE.r + (WHITE.r - SKYBLUE.r) * factor),
                static_cast<unsigned char>(SKYBLUE.g + (WHITE.g - SKYBLUE.g) * factor),
                static_cast<unsigned char>(SKYBLUE.b + (WHITE.b - SKYBLUE.b) * factor),
                255
            };
            for (int x = 0; x < screenWidth; x++) {
                ImageDrawPixel(&backgroundImg, x, y, color);
            }
        }
        backgroundTexture = LoadTextureFromImage(backgroundImg);
        UnloadImage(backgroundImg);
    }
    
    // Initialize player
    Player player = {
        {100, screenHeight - tileSize - playerTexture.height * batScale},  // position adjusted to be on top of the floor
        {0, 0},                     // velocity
        playerSpeed,                // speed
        false,                      // isJumping
        {100, screenHeight - tileSize - playerTexture.height * batScale, playerTexture.width * batScale, playerTexture.height * batScale},  // bounds updated to match bat scale
        true,                       // isFacingRight
        playerTexture,              // texture
        0                           // invulnerabilityTimer
    };
    
    // Initialize bat enemy
    Bat bat = {
        {static_cast<float>(screenWidth / 2), 100.0f},     // position
        {batSpeed, batSpeed * 0.5f},              // velocity - ensure it has vertical movement
        {0, 0, 0, 0},               // bounds (will be updated)
        batTexture,                 // texture (first frame)
        batScale,                   // scale
        true,                       // active
        0,                          // respawnTimer
        {batFrames[0], batFrames[1], batFrames[2], batFrames[3], 
         batFrames[4], batFrames[5], batFrames[6], batFrames[7]}, // animation frames
        0,                          // currentFrame
        0,                          // frameCounter
        5                           // framesSpeed (adjust for desired animation speed)
    };
    
    // Initialize score
    Score score = {0, 0};
    
    // Create ground/platforms
    std::vector<Platform> platforms;
    int numTiles = screenWidth / tileSize + 1;
    
    // Add ground tiles
    for(int i = 0; i < numTiles; i++) {
        Platform platform = {
            {i * tileSize, screenHeight - tileSize, tileSize, tileSize},
            groundTexture
        };
        platforms.push_back(platform);
    }
    
    // Add some platforms for jumping onto
    platforms.push_back({{300, screenHeight - 200, tileSize * 3, tileSize}, groundTexture});
    platforms.push_back({{600, screenHeight - 300, tileSize * 2, tileSize}, groundTexture});
    platforms.push_back({{900, screenHeight - 250, tileSize * 4, tileSize}, groundTexture});
    
    // Reserve space for stairs
    platforms.reserve(platforms.size() + 8); // Reserve space for up to 8 stairs
    
    // Generate random stairs and get the position for the flag
    Vector2 flagPosition = GenerateRandomStairs(platforms, groundTexture);
    
    // Initialize flag at the top of the stairs
    Flag flag = {
        flagPosition,
        {flagPosition.x, flagPosition.y, 40, 60},
        flagTexture,
        false, // not reached yet
        0.0f,  // wave timer
        YELLOW // color
    };
    
    // Update bat bounds based on scaled dimensions
    bat.bounds.width = bat.GetScaledWidth();
    bat.bounds.height = bat.GetScaledHeight();
    bat.bounds.x = bat.position.x;
    bat.bounds.y = bat.position.y;
    
    // Load music
    Music gameMusic = { 0 };
    bool musicLoaded = false;
    
    if (FileExists(musicPath.c_str())) {
        std::cout << "Loading music from: " << musicPath << std::endl;
        gameMusic = LoadMusicStream(musicPath.c_str());
        musicLoaded = true;
        
        // Set music volume and play it
        SetMusicVolume(gameMusic, 0.5f);  // 50% volume
        PlayMusicStream(gameMusic);
    } else {
        std::cout << "Music file not found at: " << musicPath << std::endl;
    }
    
    // Game state
    bool gameOver = false;
    
    // Game loop
    while (!WindowShouldClose()) {
        // Update music if loaded
        if (musicLoaded) {
            UpdateMusicStream(gameMusic);
        }
        
        // Update
        if (!gameOver) {
            // Handle player movement
            if (IsKeyDown(KEY_RIGHT)) {
                player.position.x += player.speed;
                player.isFacingRight = true;
            }
            if (IsKeyDown(KEY_LEFT)) {
                player.position.x -= player.speed;
                player.isFacingRight = false;
            }
            
            // Handle jumping
            if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP)) && !player.isJumping) {
                player.velocity.y = jumpForce;
                player.isJumping = true;
            }

            // Apply gravity
            player.velocity.y += gravity;
            player.position.y += player.velocity.y;
            
            // Check if player has fallen off the sides of the screen
            if (player.position.x < -player.bounds.width * 2 || 
                player.position.x > screenWidth + player.bounds.width * 2 || 
                player.position.y > screenHeight + player.bounds.height * 2) {
                // Respawn player in the middle of the screen
                player.position.x = screenWidth / 2 - player.bounds.width / 2;
                player.position.y = screenHeight / 2 - player.bounds.height / 2;
                player.velocity.x = 0;
                player.velocity.y = 0;
            }
            
            // Ground collision - check against all platforms
            bool onGround = false;
            for(const auto& platform : platforms) {
                // Check if player is colliding with platform
                if (player.position.y + player.bounds.height > platform.rect.y &&
                    player.position.y < platform.rect.y + platform.rect.height &&
                    player.position.x + player.bounds.width > platform.rect.x &&
                    player.position.x < platform.rect.x + platform.rect.width) {
                    
                    // Check if landing on top of platform (only if falling)
                    if (player.velocity.y > 0 && 
                        player.position.y + player.bounds.height - player.velocity.y <= platform.rect.y + 5) {
                        player.position.y = platform.rect.y - player.bounds.height;
                        player.velocity.y = 0;
                        onGround = true;
                    }
                    // Check if hitting platform from below
                    else if (player.velocity.y < 0 && 
                             player.position.y - player.velocity.y >= platform.rect.y + platform.rect.height - 5) {
                        player.position.y = platform.rect.y + platform.rect.height;
                        player.velocity.y = 0;
                    }
                    // Check if hitting platform from the sides
                    else if (player.position.x + player.bounds.width > platform.rect.x && 
                             player.position.x < platform.rect.x + platform.rect.width) {
                        // Left side collision
                        if (player.velocity.x > 0 && 
                            player.position.x + player.bounds.width - player.velocity.x <= platform.rect.x + 5) {
                            player.position.x = platform.rect.x - player.bounds.width;
                        }
                        // Right side collision
                        else if (player.velocity.x < 0 && 
                                 player.position.x - player.velocity.x >= platform.rect.x + platform.rect.width - 5) {
                            player.position.x = platform.rect.x + platform.rect.width;
                        }
                    }
                }
            }
            
            player.isJumping = !onGround;

            // Update player bounds
            player.bounds.x = player.position.x;
            player.bounds.y = player.position.y;
            
            // Update invulnerability timer
            if (player.invulnerabilityTimer > 0) {
                player.invulnerabilityTimer--;
            }
            
            // Update bat movement
            if (bat.active) {
                // Update respawn timer
                if (bat.respawnTimer > 0) {
                    bat.respawnTimer--;
                }
                
                // Update animation
                bat.frameCounter++;
                if (bat.frameCounter >= bat.framesSpeed) {
                    bat.frameCounter = 0;
                    bat.currentFrame++;
                    
                    if (bat.currentFrame > 7) bat.currentFrame = 0;
                    
                    // Update the current texture to the current frame
                    bat.texture = bat.frames[bat.currentFrame];
                }
                
                // Move bat
                bat.position.x += bat.velocity.x;
                bat.position.y += bat.velocity.y;
                
                // Bounce off screen edges
                if (bat.position.x <= 0 || bat.position.x + bat.GetScaledWidth() >= screenWidth) {
                    bat.velocity.x *= -1;
                }
                if (bat.position.y <= 0 || bat.position.y + bat.GetScaledHeight() >= screenHeight) {
                    bat.velocity.y *= -1;
                }
                
                // Update bat bounds
                bat.bounds.x = bat.position.x;
                bat.bounds.y = bat.position.y;
                
                // Check collision with player only if player is not invulnerable and bat can respawn
                if (player.invulnerabilityTimer <= 0 && bat.respawnTimer <= 0 && 
                    CheckCollisionRecs(player.bounds, bat.bounds)) {
                    // Increment bat score
                    score.bat++;
                    
                    // Make player invulnerable for a short time
                    player.invulnerabilityTimer = invulnerabilityFrames;
                    
                    // Respawn bat in a random position away from player
                    RespawnBat(bat, player);
                    
                    // Set bat respawn timer to prevent immediate respawn
                    bat.respawnTimer = 30; // Half a second at 60 FPS
                }
            }

            // Update flag wave animation
            flag.waveTimer += 0.05f;
        }

        // Check if player reached the flag
        if (!flag.reached && CheckCollisionRecs(player.bounds, flag.bounds)) {
            flag.reached = true;
            score.player++; // Increment player score when flag is reached
            
            // Make the flag wave faster when reached
            flag.waveTimer = 0.0f;
            flag.color = GREEN; // Change flag color to green when reached
            
            // Set game over state
            gameOver = true;
        }

        // Draw
        BeginDrawing();
        
        // Draw background
        DrawTexture(backgroundTexture, 0, 0, WHITE);
        
        // Draw platforms
        for(const auto& platform : platforms) {
            DrawTexture(platform.texture, platform.rect.x, platform.rect.y, WHITE);
        }
        
        // Draw flag with waving animation
        if (FileExists(flagImagePath.c_str())) {
            // If we have a flag texture, draw it
            DrawTexture(flag.texture, flag.position.x, flag.position.y, flag.color);
        } else {
            // Draw a simple animated flag
            for (int i = 0; i < 5; i++) {
                float waveOffset = sinf(flag.waveTimer + i * 0.3f) * 5.0f;
                DrawRectangle(flag.position.x, flag.position.y + i * 10 + waveOffset, 30, 8, flag.color);
            }
            // Draw flag pole
            DrawRectangle(flag.position.x - 5, flag.position.y, 5, 60, DARKGRAY);
        }
        
        // Draw player - flip texture based on direction and flash if invulnerable
        if (player.invulnerabilityTimer > 0 && (player.invulnerabilityTimer / 5) % 2 == 0) {
            // Skip drawing player every few frames to create flashing effect
        } else {
            if (player.isFacingRight) {
                // Draw player with the same scale as the bat
                DrawTextureEx(player.texture, player.position, 0.0f, batScale, WHITE);
            } else {
                // Draw flipped texture when facing left with the same scale as the bat
                // Use negative scale for horizontal flipping
                DrawTextureEx(player.texture, {player.position.x + player.bounds.width, player.position.y}, 0.0f, -batScale, WHITE);
            }
        }
        
        // Draw bat enemy (if active)
        if (bat.active) {
            if (usingBatFallback) {
                // Draw a more visible bat if using fallback
                DrawRectangle(bat.position.x, bat.position.y, bat.bounds.width, bat.bounds.height, RED);
                DrawRectangleLines(bat.position.x, bat.position.y, bat.bounds.width, bat.bounds.height, BLACK);
            } else {
                // Draw the current animation frame
                DrawTextureEx(bat.frames[bat.currentFrame], bat.position, 0.0f, bat.scale, WHITE);
            }
            
            // Draw debug info for bat bounds
            DrawRectangleLines(bat.bounds.x, bat.bounds.y, bat.bounds.width, bat.bounds.height, PURPLE);
        }
        
        // Draw game info
        DrawText("Arrow Keys: Move | Space/Up: Jump", 20, 20, 20, BLACK);
        
        // Draw debug info about the player texture
        char debugInfo[100];
        sprintf(debugInfo, "Player Texture: %dx%d", player.texture.width, player.texture.height);
        DrawText(debugInfo, 20, 50, 20, RED);

        // Draw score
        char scoreText[100];
        sprintf(scoreText, "BAT: %d | PLAYER: %d", score.bat, score.player);
        DrawText(scoreText, screenWidth - 250, 20, 20, BLACK);
        
        // Draw flag status if reached
        if (flag.reached) {
            DrawText("FLAG REACHED!", flag.position.x - 50, flag.position.y - 30, 16, GREEN);
        }
        
        // Draw invulnerability status if active
        if (player.invulnerabilityTimer > 0) {
            DrawText("INVULNERABLE!", player.position.x, player.position.y - 20, 16, YELLOW);
        }

        // Draw game over screen if flag is reached
        if (gameOver) {
            // Draw semi-transparent overlay
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            
            // Draw "YOU WIN FREER" text
            const char* winText = "YOU WIN FREER";
            int fontSize = 60;
            int textWidth = MeasureText(winText, fontSize);
            DrawText(winText, screenWidth/2 - textWidth/2, screenHeight/2 - fontSize/2, fontSize, YELLOW);
            
            // Draw instruction to exit
            const char* exitText = "Press ESC to exit";
            int smallFontSize = 30;
            int smallTextWidth = MeasureText(exitText, smallFontSize);
            DrawText(exitText, screenWidth/2 - smallTextWidth/2, screenHeight/2 + fontSize, smallFontSize, WHITE);
        }

        EndDrawing();
    }

    // De-initialization
    UnloadTexture(playerTexture);
    UnloadTexture(groundTexture);
    UnloadTexture(backgroundTexture);
    UnloadTexture(flagTexture);
    
    // Unload all bat animation frames
    if (!usingBatFallback) {
        for (int i = 0; i < 8; i++) {
            UnloadTexture(bat.frames[i]);
        }
    } else {
        UnloadTexture(batTexture);
    }
    
    // Unload music if loaded
    if (musicLoaded) {
        UnloadMusicStream(gameMusic);
    }
    
    CloseAudioDevice();
    CloseWindow();
    return 0;
} 