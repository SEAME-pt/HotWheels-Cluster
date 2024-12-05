#include <iostream>
#include <SDL2/SDL.h>
#include <thread>
#include <atomic>
#include <csignal>
#include "Jetcar.hpp"

volatile bool keepRunning = true;

// Signal handler function
/**
 * Signal handler that sets the `keepRunning` flag to false when a termination signal is received.
 * 
 * - `signum`: The signal number (e.g., SIGTERM, SIGINT).
 * 
 * This function prints a message indicating the program has been stoped,
 * and stops the running process by setting `keepRunning` to false.
 */
void signalHandler(int signum) {
    std::cout << "Signal (" << signum << ") received. Stopping gracefully..." << std::endl;
    keepRunning = false;
}

/**
 * Main function that initializes and controls the Jetcar.
 * 
 * - Sets up signal handlers for termination signals (SIGTERM and SIGINT).
 * - Creates a Jetcar object with the specified servo and motor I2C addresses.
 * - Starts the Jetcar and continuously runs until a termination signal is received.
 * - Stops the Jetcar and handles any exceptions that may occur during initialization or operation.
 * 
 * The function ensures proper cleanup before exiting and prints an error message if an exception is thrown.
 */
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