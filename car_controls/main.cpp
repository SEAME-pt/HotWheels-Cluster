#include <iostream>
#include <SDL2/SDL.h>
#include <thread>
#include <atomic>
#include <csignal>
#include "Jetcar.hpp"

volatile bool keepRunning = true;

// Global atomic flag to signal the event thread to stop
/* std::atomic<bool> running(true);

SDL_Joystick *initJoystick() {
   // Initialize SDL
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL Initialization failed: " << SDL_GetError() << std::endl;
        exit (1);
    }
    // Open the joystick
    if (SDL_NumJoysticks() < 1) {
        std::cerr << "No joystick connected!" << std::endl;
        SDL_Quit();
        exit (1);
    }
    SDL_Joystick* joystick = SDL_JoystickOpen(0);  // Open the first joystick
    if (!joystick) {
        std::cerr << "Joystick could not be opened: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit (1);
    }
    return (joystick);
}

void    controllerLoop(SDL_Joystick* joystick) {
    
    // Event loop
    bool running = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            // Handle joystick button press event
            if (e.type == SDL_JOYBUTTONDOWN) {
                std::cout << "Button " << (int)e.jbutton.button << " Pressed!" << std::endl;
            }
            // Handle joystick axis motion event
            if (e.type == SDL_JOYAXISMOTION) {
                if ((int)e.jaxis.axis == 0)
                    std::cout << "Car direction" << " moved to position " << e.jaxis.value << std::endl;
                if ((int)e.jaxis.axis == 3)
                    std::cout << "Throttle" << " moved to position " << e.jaxis.value << std::endl;
            }
        }
    }
} */

/* int main(int argc, char* argv[]) {

    //Start the joystick
    SDL_Joystick* joystick = initJoystick();
    // Start the event loop in a separate thread
    std::thread controllerThread([joystick]() {
        controllerLoop(joystick);
    });

    // Main program can do other tasks here, if needed
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get(); // Wait for the user to press Enter to stop the program

    // Stop the event loop by setting the running flag to false
    running = false;

    // Wait for the event loop thread to finish
    controllerThread.join();
    // Close joystick and clean up SDL
    SDL_JoystickClose(joystick);
    SDL_Quit();

    return 0;
} */

// Signal handler function
void signalHandler(int signum) {
    std::cout << "Signal (" << signum << ") received. Stopping gracefully..." << std::endl;
    keepRunning = false;
}

int main() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    try {
        Jetcar car(0x40, 0x60);
        car.start();
        while (keepRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        car.stop();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Service has stopped." << std::endl;
    return 0;
}