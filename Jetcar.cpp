#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>  // Add this header
#include <cmath>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <SDL2/SDL.h>

class JetCar {
private:
    // I2C bus file descriptors
    int servo_fd, motor_fd;
    
    // Configuration constants
    static constexpr int SERVO_ADDR = 0x40;
    static constexpr int MOTOR_ADDR = 0x60;
    static constexpr int STEERING_CHANNEL = 0;
    static constexpr int MAX_ANGLE = 180;
    static constexpr double RODA_DIAMETRO = 0.065;
    static constexpr int FUROS = 36;

    // State variables with proper class scope
    std::atomic<bool> running{false};
    std::atomic<double> current_speed{0.0};
    std::atomic<double> current_angle{0.0};
    std::thread gamepad_thread;

    // Velocity tracking
    std::atomic<int> pulsos{0};
    std::chrono::steady_clock::time_point last_time;

    void init_i2c() {
        // Open I2C bus devices
        servo_fd = open("/dev/i2c-1", O_RDWR);
        motor_fd = open("/dev/i2c-1", O_RDWR);

        if (servo_fd < 0 || motor_fd < 0) {
            throw std::runtime_error("Failed to open I2C bus");
        }

        // Set device addresses
        ioctl(servo_fd, I2C_SLAVE, SERVO_ADDR);
        ioctl(motor_fd, I2C_SLAVE, MOTOR_ADDR);
    }

    void init_servo() {
        // Low-level servo initialization sequence
        uint8_t init_sequence[] = {0x00, 0x06, 0x00, 0x10, 0xFE, 0x79};
        write(servo_fd, init_sequence, sizeof(init_sequence));
    }

    void set_servo_pwm(int channel, int on_value, int off_value) {
        uint8_t pwm_data[4] = {
            static_cast<uint8_t>(on_value & 0xFF),
            static_cast<uint8_t>(on_value >> 8),
            static_cast<uint8_t>(off_value & 0xFF),
            static_cast<uint8_t>(off_value >> 8)
        };
        
        int base_reg = 0x06 + (channel * 4);
        for (int i = 0; i < 4; ++i) {
            write(servo_fd, &pwm_data[i], 1);
        }
    }

    void process_gamepad() {
        while (running.load()) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_JOYBUTTONDOWN:
                        // Handle button events
                        break;
                    case SDL_JOYAXISMOTION:
                        // Handle axis motion for steering and speed
                        break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

public:
    JetCar() {
        try {
            init_i2c();
            init_servo();
        } catch (const std::exception& e) {
            std::cerr << "Initialization error: " << e.what() << std::endl;
        }
    }

    void start() {
        running.store(true);
        gamepad_thread = std::thread(&JetCar::process_gamepad, this);
    }

    void stop() {
        running.store(false);
        if (gamepad_thread.joinable()) {
            gamepad_thread.join();
        }
    }

    double calculate_velocity() {
        int current_pulsos = pulsos.load();
        double voltas = current_pulsos / static_cast<double>(FUROS);
        double distancia = voltas * (RODA_DIAMETRO * M_PI);
        double velocidade_ms = distancia / 1.0;  // 1-second interval
        return velocidade_ms * 3.6;  // Convert to km/h
    }

    ~JetCar() {
        stop();
        close(servo_fd);
        close(motor_fd);
    }
};

int main() {
    try {
        JetCar car;
        car.start();

        // Main loop
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}