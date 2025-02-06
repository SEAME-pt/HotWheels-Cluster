#include <Ice/Ice.h>
#include "Joystick.h"  // Generated by slice2cpp
#include <iostream>
#include <mutex>

class CarDataI : public Data::CarData {
private:
    bool joystick_enable;
    double car_temperature;

    std::mutex joystick_mutex;
    std::mutex temperature_mutex;

public:
    CarDataI() : joystick_enable(true), car_temperature(0.0) {}

    void setJoystickValue(bool newValue, const Ice::Current&) override {
        std::lock_guard<std::mutex> lock(joystick_mutex);
        joystick_enable = newValue;
        std::cout << "Joystick value set to: " << joystick_enable << std::endl;
    }

    bool getJoystickValue(const Ice::Current&) override {
        std::lock_guard<std::mutex> lock(joystick_mutex);
        return joystick_enable;
    }

    void setCarTemperatureValue(double newValue, const Ice::Current&) override {
        std::lock_guard<std::mutex> lock(temperature_mutex);
        car_temperature = newValue;
        std::cout << "Car temperature value set to: " << car_temperature << std::endl;
    }

    double getCarTemperatureValue(const Ice::Current&) override {
        std::lock_guard<std::mutex> lock(temperature_mutex);
        return car_temperature;
    }
};

int main(int argc, char* argv[]) {
    try {
        Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints(
            "CarDataAdapter", "tcp -h 127.0.0.1 -p 10000");

        // Create a thread pool to allow concurrent request processing
        adapter->addServantLocator(new Ice::ThreadPool(10));

        CarDataI* servant = new CarDataI();
        adapter->add(servant, Ice::stringToIdentity("carData"));
        adapter->activate();

        std::cout << "Server is running, press Ctrl+C to stop." << std::endl;
        communicator->waitForShutdown();
    }
    catch (const Ice::Exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
