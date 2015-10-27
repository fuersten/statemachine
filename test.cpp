
#include "statemachine.h"


class StartEvent : public statemachine::Event
{  
};

class StopEvent : public statemachine::Event
{  
};

class QuitEvent : public statemachine::Event
{
};

class CountingContext : public statemachine::Context
{
public:
    CountingContext()
    : mCounter(0)
    {
    }
    
    int mCounter;
};


class StartState : public statemachine::State<CountingContext>
{
public:
    virtual void run(const statemachine::Event::Ptr& event)
    {
        std::cout << "StartState::run" << std::endl;
        
        addEvent(statemachine::Event::Ptr(new StartEvent));
    }
    
    virtual void onEntry(const statemachine::Event::Ptr& event)
    {
        std::cout << "StartState::onEntry" << std::endl;
    }
    
    virtual void onExit(const statemachine::Event::Ptr& event)
    {
        std::cout << "StartState::onExit" << std::endl;
    }
};

class CountState : public statemachine::State<CountingContext>
{
public:    
    virtual void run(const statemachine::Event::Ptr& event)
    {
        std::cout << "CountState::run" << std::endl;
        ++mContext->mCounter;
    }
    
    virtual void onEntry(const statemachine::Event::Ptr& event)
    {
        std::cout << "CountState::onEntry" << std::endl;
    }
    
    virtual void OnExit(const statemachine::Event::Ptr& event)
    {
        std::cout << "CountState::onExit" << std::endl;
    }
};

class StopState : public statemachine::State<CountingContext>
{
public:
    virtual void run(const statemachine::Event::Ptr& event)
    {
        std::cout << "StopState::run" << std::endl;
    }
    
    virtual void onEntry(const statemachine::Event::Ptr& event)
    {
        std::cout << "StopState::onEntry" << std::endl;
    }
    
    virtual void onExit(const statemachine::Event::Ptr& event)
    {
        std::cout << "StopState::onExit" << std::endl;
    }
};

class QuitState : public statemachine::ExitState<CountingContext>
{
public:
    virtual void onEntry(const statemachine::Event::Ptr& event)
    {
        std::cout << "QuitState::onEntry" << std::endl;
    }
};

class CountingSM : public statemachine::StateMachine<StartState,CountingContext>
{
public:
    typedef std::shared_ptr<CountingSM> Ptr;
    
    CountingSM()
    : StateMachine<StartState,CountingContext>("CountingSM")
    {
    }
    
    virtual void doSetUp()
    {
        SM_ADD_TRANSITION(StartState, CountState, StartEvent);
        SM_ADD_TRANSITION(CountState, StopState, StopEvent);
        SM_ADD_TRANSITION(StopState, CountState, StartEvent);
        SM_ADD_TRANSITION(StartState, QuitState, QuitEvent);
        SM_ADD_TRANSITION(CountState, QuitState, QuitEvent);
        SM_ADD_TRANSITION(StopState, QuitState, QuitEvent);
    }
};



int main(int argc, char** argv)
{
    CountingSM::Ptr sm(new CountingSM);
    
    sm->start();
    sm->processEvent(statemachine::Event::Ptr(new StopEvent));
    sm->processEvent(statemachine::Event::Ptr(new StartEvent));
    sm->processEvent(statemachine::Event::Ptr());
    sm->processEvent(statemachine::Event::Ptr(new StopEvent));
    sm->processEvent(statemachine::Event::Ptr(new QuitEvent));
    sm->processEvent(statemachine::Event::Ptr());
    
    std::cout << "Counter: " << sm->context()->mCounter << std::endl;
    if(sm->context()->mCounter != 3) {
        std::cerr << "Wrong counter, should have been 3!" << std::endl;
    }
    
    std::cout << "Statemachine graph:" << std::endl;
    std::cout << sm->generateDot();
    
    return 0;
}
