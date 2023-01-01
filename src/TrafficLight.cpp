#include "TrafficLight.h"
#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */

template <typename T> T MessageQueue<T>::receive() {
  std::unique_lock<std::mutex> guard(_queue_mtx);
  _cond.wait(guard, [this]() { return !_queue.empty(); });
  T msg = std::move(_queue.back());
  _queue.pop_back();

  return msg;
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and
  // _condition.wait() to wait for and receive new messages and pull them from
  // the queue using move semantics. The received object should then be returned
  // by the receive function.
}

template <typename T> void MessageQueue<T>::send(T &&msg) {
  std::lock_guard<std::mutex> guard(_queue_mtx);
  _queue.clear(); /// CHECK: Leave this line??
  _queue.emplace_back(std::move(msg));
  _cond.notify_one();
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new
  // message to the queue and afterwards send a notification.
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() {
  _queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
  _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen() {
  while (_queue->receive() != TrafficLightPhase::green) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return;
  // FP.5b : add the implementation of the method waitForGreen, in which an
  // infinite while-loop
  //  runs and repeatedly calls the receive function on the message queue.
  //  Once it receives TrafficLightPhase::green, the method returns.
}

TrafficLightPhase TrafficLight::getCurrentPhase() { return _currentPhase; }

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started
  // in a thread when the public method „simulate“ is called. To do this, use
  // the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {

  // FP.2a : Implement the function with an infinite loop that measures the time
  // between two loop cycles and toggles the current phase of the traffic light
  // between red and green and sends an update method to the message queue using
  // move semantics. The cycle duration should be a random value between 4 and 6
  // seconds. Also, the while-loop should use std::this_thread::sleep_for to
  // wait 1ms between two cycles.

  std::random_device rdmDev;
  std::mt19937 range(rdmDev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(4, 6);

  float PhaseDuration = 5;
  std::chrono::time_point<std::chrono::system_clock> LastSwitch =
      std::chrono::system_clock::now();
  ;
  std::chrono::microseconds delta;

  while (true) {
    delta = std::chrono::system_clock::now() - LastSwitch;
    if (delta.count() / 1000000 > PhaseDuration) {
      _currentPhase = (_currentPhase == TrafficLightPhase::red)
                          ? TrafficLightPhase::green
                          : TrafficLightPhase::red;
      PhaseDuration =
          dist(range); // set randomly Phase duration in 10th of seconds
      LastSwitch = std::chrono::system_clock::now();
      std::unique_lock<std::mutex> lck(_mtx);
      std::cout << "New cycle ("
                << ((_currentPhase == TrafficLightPhase::red) ? "red" : "green")
                << ") Duration: " << PhaseDuration << "\n";
      lck.unlock();
      TrafficLightPhase phase_copy = _currentPhase;
      _queue.get()->send(std::move(phase_copy));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}
