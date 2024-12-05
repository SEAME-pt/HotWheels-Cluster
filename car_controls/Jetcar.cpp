#include "Jetcar.hpp"

/* This is to work and write on the bus */

/**
 * Represents data formats for I2C SMBus communication:
 * - `byte`: Single byte of data.
 * - `word`: 16-bit (2 byte) data.
 * - `block`: Array for up to 34-byte block transfers.
 */
union i2c_smbus_data {
    uint8_t byte;
    uint16_t word; 
    uint8_t block[34]; // Block size for SMBus
};

/* ------------------------------------ */

#define I2C_SMBUS_WRITE 0
#define I2C_SMBUS_READ  1
#define I2C_SMBUS_BYTE_DATA 2

/**
 * Writes a single byte of data to a specified I2C device register.
 * 
 * - `file`: File descriptor for the I2C device.
 * - `command`: The register address or command to write to.
 * - `value`: The byte of data to write to the register.
 *
 * @return Uses the SMBus protocol to send the data via an ioctl system call.
 */
int i2c_smbus_write_byte_data(int file, uint8_t command, uint8_t value) {
    union i2c_smbus_data data;
    data.byte = value;

    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_WRITE;
    args.command = command;
    args.size = I2C_SMBUS_BYTE_DATA;
    args.data = &data;

    return ioctl(file, I2C_SMBUS, &args);
}

/**
 * Reads a single byte of data from a specified I2C device register.
 * 
 * - `file`: File descriptor for the I2C device.
 * - `command`: The register address or command to read from.
 *
 * @return The byte read from the device, or -1 if an error occurs.
 */
int i2c_smbus_read_byte_data(int file, uint8_t command) {
    union i2c_smbus_data data;

    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_READ;
    args.command = command;
    args.size = I2C_SMBUS_BYTE_DATA;
    args.data = &data;

    if (ioctl(file, I2C_SMBUS, &args) < 0) {
        return -1;
    }
    return data.byte;
}

/**
 * Clamps a value within the specified range.
 * 
 * - `value`: The value to clamp.
 * - `min_val`: The minimum allowed value.
 * - `max_val`: The maximum allowed value.
 * 
 * Returns `value` if it is within the range [min_val, max_val],
 * otherwise returns `min_val` or `max_val` if the value is outside the range.
 */
template <typename T>
T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : ((value > max_val) ? max_val : value);
}
/* ---------------------------------------------------- */

Jetcar::Jetcar() {}

/**
 * Constructor for the Jetcar class.
 * 
 * Initializes the Jetcar object with optional I2C addresses for the servo and motor.
 * 
 * - `servo_addr`: The I2C address for the servo (default is 0x40).
 * - `motor_addr`: The I2C address for the motor (default is 0x60).
 * 
 * Opens the I2C buses, sets the addresses for the servo and motor, and initializes them.
 * Throws exceptions if the I2C devices cannot be opened or addressed.
 */
Jetcar::Jetcar(int servo_addr = 0x40, int motor_addr = 0x60)
    : servo_addr_(servo_addr), motor_addr_(motor_addr), running_(false), current_speed_(0), current_angle_(0) {

    // Initialize I2C buses
    servo_bus_fd_ = open("/dev/i2c-1", O_RDWR);
    motor_bus_fd_ = open("/dev/i2c-1", O_RDWR);

    if (servo_bus_fd_ < 0 || motor_bus_fd_ < 0) {
        throw std::runtime_error("Failed to open I2C device");
    }

    // Set addresses
    if (ioctl(servo_bus_fd_, I2C_SLAVE, servo_addr_) < 0 || ioctl(motor_bus_fd_, I2C_SLAVE, motor_addr_) < 0) {
        throw std::runtime_error("Failed to set I2C address");
    }

    init_servo();
    init_motors();
}

/**
 * Destructor for the Jetcar class.
 * 
 * Stops the car's movement and closes the I2C buses for the servo and motor.
 */
Jetcar::~Jetcar() {
    stop();
    close(servo_bus_fd_);
    close(motor_bus_fd_);
}

/**
 * Starts the Jetcar by setting it to the running state and launching a thread
 * to process joystick input.
 */
void Jetcar::start() {
    running_ = true;
    joystick_thread_ = std::thread(&Jetcar::process_joystick, this);
}

/**
 * Stops the Jetcar by setting it to the stopped state, joining the joystick input
 * thread if it is running, and resetting the speed and steering to zero.
 */
void Jetcar::stop() {
    running_ = false;
    if (joystick_thread_.joinable()) {
        joystick_thread_.join();
    }
    set_speed(0);
    set_steering(0);
}

/**
 * Sets the speed of the Jetcar by adjusting the PWM values for the motors.
 * 
 * - `speed`: The desired speed, clamped between -100 and 100.
 * Negative values move the car backward, positive values move it forward, and zero stops the car.
 * 
 * The function adjusts the appropriate motor channels for forward, backward, or stop movement
 * based on the clamped speed value.
 */
void Jetcar::set_speed(int speed) {
    speed = clamp(speed, -100, 100);
    int pwm_value = static_cast<int>(std::abs(speed) / 100.0 * 4096);

    if (speed < 0) { // Backward
        //left motor
        set_motor_pwm(0, pwm_value);
        set_motor_pwm(1, pwm_value);
        set_motor_pwm(2, 0);
        //right motor
        set_motor_pwm(6, pwm_value);
        set_motor_pwm(7, pwm_value);
        set_motor_pwm(8, 0);
    } else if (speed > 0) { // Forward
        //left motor
        set_motor_pwm(0, pwm_value);
        set_motor_pwm(1, 0);
        set_motor_pwm(2, pwm_value);
        //right motor
        set_motor_pwm(5, pwm_value);
        set_motor_pwm(6, 0);
        set_motor_pwm(7, pwm_value);
    } else { // Stop
        for (int channel = 0; channel < 9; ++channel)
            set_motor_pwm(channel, 0);
    }
    current_speed_ = speed;
}

/**
 * Sets the steering angle of the Jetcar by adjusting the PWM value for the servo.
 * 
 * - `angle`: The desired steering angle, clamped between -MAX_ANGLE and MAX_ANGLE.
 *   Negative values steer left, positive values steer right, and zero centers the steering.
 * 
 * The function calculates the corresponding PWM value for the servo and applies it to the steering channel.
 */
void Jetcar::set_steering(int angle) {
    angle = clamp(angle, -MAX_ANGLE, MAX_ANGLE);
    int pwm = 0;
    if (angle < 0) {
        pwm = SERVO_CENTER_PWM + static_cast<int>((angle / static_cast<float>(MAX_ANGLE)) * (SERVO_CENTER_PWM - SERVO_LEFT_PWM));
    } else if (angle > 0) {
        pwm = SERVO_CENTER_PWM + static_cast<int>((angle / static_cast<float>(MAX_ANGLE)) * (SERVO_RIGHT_PWM - SERVO_CENTER_PWM));
    } else {
        pwm = SERVO_CENTER_PWM;
    }

    set_servo_pwm(STEERING_CHANNEL, 0, pwm);
    current_angle_ = angle;
}

/**
 * Initializes the servo by sending a series of configuration commands over I2C.
 * 
 * This function configures the servo controller with specific initialization commands,
 * including setting registers and applying delays to ensure proper initialization.
 */
void Jetcar::init_servo() {
    write_byte_data(servo_bus_fd_, 0x00, 0x06);
    usleep(100000);

    write_byte_data(servo_bus_fd_, 0x00, 0x10);
    usleep(100000);

    write_byte_data(servo_bus_fd_, 0xFE, 0x79);
    usleep(100000);

    write_byte_data(servo_bus_fd_, 0x01, 0x04);
    usleep(100000);

    write_byte_data(servo_bus_fd_, 0x00, 0x20);
    usleep(100000);
}

/**
 * Initializes the motors by configuring the motor controller over I2C.
 * 
 * This function sets up the motor controller by writing specific initialization commands, 
 * adjusting the prescale value for the PWM frequency, and setting the operating mode.
 * It also includes a small delay to ensure proper motor initialization.
 */
void Jetcar::init_motors() {
    write_byte_data(motor_bus_fd_, 0x00, 0x20);

    int prescale = static_cast<int>(std::floor(25000000.0 / 4096.0 / 100 - 1));
    int oldmode = read_byte_data(motor_bus_fd_, 0x00);
    int newmode = (oldmode & 0x7F) | 0x10;

    write_byte_data(motor_bus_fd_, 0x00, newmode);
    write_byte_data(motor_bus_fd_, 0xFE, prescale);
    write_byte_data(motor_bus_fd_, 0x00, oldmode);
    usleep(5000);
    write_byte_data(motor_bus_fd_, 0x00, oldmode | 0xa1);
}

/**
 * Sets the PWM value for a specific servo channel.
 * 
 * - `channel`: The servo channel to control.
 * - `on_value`: The "on" time for the PWM signal (low byte and high byte).
 * - `off_value`: The "off" time for the PWM signal (low byte and high byte).
 * 
 * This function writes the corresponding PWM values to the servo controller's registers
 * over I2C to adjust the position of the servo.
 */
void Jetcar::set_servo_pwm(int channel, int on_value, int off_value) {
    int base_reg = 0x06 + (channel * 4);
    write_byte_data(servo_bus_fd_, base_reg, on_value & 0xFF);
    write_byte_data(servo_bus_fd_, base_reg + 1, on_value >> 8);
    write_byte_data(servo_bus_fd_, base_reg + 2, off_value & 0xFF);
    write_byte_data(servo_bus_fd_, base_reg + 3, off_value >> 8);
}

/**
 * Sets the PWM value for a specific motor channel.
 * 
 * - `channel`: The motor channel to control.
 * - `value`: The PWM value to set (clamped between 0 and 4096).
 * 
 * This function writes the PWM value to the motor controller's registers over I2C
 * to control the motor's speed and direction.
 */
void Jetcar::set_motor_pwm(int channel, int value) {
    value = clamp(value, 0, 4096);
    int base_reg = 0x06 + (4 * channel);
    write_byte_data(motor_bus_fd_, base_reg, value & 0xFF);
    write_byte_data(motor_bus_fd_, base_reg + 1, value >> 8);
}

/**
 * Writes a single byte of data to a specified I2C register.
 * 
 * - `fd`: The file descriptor for the I2C device.
 * - `reg`: The register address to write to.
 * - `value`: The byte value to write to the register.
 * 
 * This function performs an I2C write operation and throws an exception if the write fails.
 */
void Jetcar::write_byte_data(int fd, int reg, int value) {
    if (i2c_smbus_write_byte_data(fd, reg, value) < 0) {
        throw std::runtime_error("I2C write failed");
    }
}

/**
 * Reads a single byte of data from a specified I2C register.
 * 
 * - `fd`: The file descriptor for the I2C device.
 * - `reg`: The register address to read from.
 * 
 * This function performs an I2C read operation and throws an exception if the read fails.
 * 
 * @return The byte value read from the register.
 */
int Jetcar::read_byte_data(int fd, int reg) {
    int result = i2c_smbus_read_byte_data(fd, reg);
    if (result < 0) {
        throw std::runtime_error("I2C read failed");
    }  
    return result;
}

/**
 * Processes joystick input to control the Jetcar's steering and speed.
 * 
 * Initializes the SDL library and opens the first available joystick.
 * Continuously reads the joystick's axes (steering and throttle) while the car is running,
 * updating the car's steering and speed accordingly.
 * 
 * - Steering is mapped to the X-axis of the left joystick.
 * - Speed is mapped to the Y-axis of the right joystick.
 * 
 * The function runs in a separate thread and updates the car's controls at a set interval.
 * The joystick is closed and SDL is quit when the function ends.
 */
void Jetcar::process_joystick() {
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return;
    }

    SDL_Joystick* joystick = SDL_JoystickOpen(0);
    if (!joystick) {
        /* std::cerr << "Failed to open joystick: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return; */
        process_joystick();
    }

    while (running_) {
        //this is just to test if the controller is recognized
        /* SDL_Event e;
        int num_buttons = SDL_JoystickNumButtons(joystick);
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running_ = false;
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
            for (int i = 0; i < SDL_JoystickNumButtons(joystick); ++i) {
                if (SDL_JoystickGetButton(joystick, i)) {
                    printf("Button %d is pressed\n", i);
                }
            }
        } */

        SDL_JoystickUpdate();
        //button 8 is L2
        //button 9 is R2
        int left_joystick_x = SDL_JoystickGetAxis(joystick, 0); // Steering
        int right_joystick_y = SDL_JoystickGetAxis(joystick, 3); // Speed

        set_steering(left_joystick_x / 32767.0 * MAX_ANGLE);
        set_speed(right_joystick_y / 32767.0 * 100);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    SDL_JoystickClose(joystick);
    SDL_Quit();
}