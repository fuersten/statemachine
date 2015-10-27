// The MIT License (MIT)
// Copyright (c) 2010 Lars Fuerstenberg
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SM_STATEMACHINE_H
#define SM_STATEMACHINE_H

#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)
#include <cxxabi.h>
#endif

namespace statemachine
{
    
    struct Demangler
    {
        static std::string demangle(const std::string& classname)
        {
            
#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)
            
            int status(0);
            std::string result;
            
            char* ss = abi::__cxa_demangle(classname.c_str(), 0, 0, &status);
            
            if(status == -1)
            {
                throw std::bad_alloc();
            }
            else if(status == -2)
            {
                result = classname;
            }
            else if(status == -3)
            {
                throw std::runtime_error("__cxa_demangle returned -3");
            }
            else
            {
                result = ss;
            }
            
            if(ss)
            {
                ::free(ss);
            }
            
            if(result[result.length()-1] == '*')
            {
                result.erase(result.length()-1);
            }
            std::string::size_type pos = 0;
            while((pos = result.find(", ", pos)) != std::string::npos)
            {
                result.erase(pos+1, 1);
            }
            
            return result;
            
#elif defined(_WIN32) || defined(_WIN64)
            
            std::string::size_type pos1 = classname.find_first_of(" ");
            std::string::size_type pos3 = classname.find_last_of(">");
            if(pos3 != std::string::npos)
            {
                return classname.substr(pos1+1, (pos3 - pos1));
            }
            std::string::size_type pos2 = classname.find_last_of(" ");
            return classname.substr(pos1+1, (pos2 - pos1)-1);
            
#else
#error("demangle not defined for platform")
#endif
        }
    };
    
    class Context
    {
    public:
        typedef std::shared_ptr<Context> Ptr;
        typedef std::weak_ptr<Context> WeakPtr;
        
        virtual ~Context()
        {}
        
    protected:
        Context()
        {}
    };
    
    
    class Event
    {
    public:
        typedef std::shared_ptr<Event> Ptr;
        
        virtual ~Event()
        {}
        
    protected:
        Event()
        {}
    };
    
    
    class StateMachineBase
    {
    public:
        typedef std::shared_ptr<StateMachineBase> Ptr;
        typedef std::weak_ptr<StateMachineBase> WeakPtr;
        
        StateMachineBase()
        {}
        
        virtual ~StateMachineBase()
        {}
        
        virtual void addEvent(const Event::Ptr& event) = 0;
    };
    
    
    template<typename TContext>
    class State
    {
    public:
        typedef std::shared_ptr<State> Ptr;
        typedef StateMachineBase::Ptr StateMachinePtr;
        typedef std::shared_ptr<TContext> ContextPtr;
        
        State()
        {}
        
        virtual ~State()
        {}
        
        void setStateMachine(const StateMachineBase::WeakPtr& stateMachine)
        {
            mStateMachine = stateMachine;
        }
        
        void setContext(const ContextPtr& context)
        {
            mContext = context;
        }
        
        void addEvent(const Event::Ptr& event)
        {
            auto machine = mStateMachine.lock();
            if(machine) {
                machine->addEvent(event);
            }
        }
        
        virtual void run(const Event::Ptr& event) = 0;
        
        virtual void onEntry(const Event::Ptr& event)
        {}
        
        virtual void onExit(const Event::Ptr& event)
        {}
        
    protected:
        ContextPtr mContext;
        StateMachineBase::WeakPtr mStateMachine;
    };
    
    
    template<typename TContext>
    class ExitState : public State<TContext>
    {
    public:
        virtual void run(const Event::Ptr& event)
        {
            // Dont override this as it is never called for an exit state
        }
    };

    
    template<typename TContext>
    class TransitionBase
    {
    public:
        typedef std::shared_ptr<TransitionBase> Ptr;
        typedef typename State<TContext>::Ptr StatePtr;
        
        virtual ~TransitionBase() {}
        
        virtual StatePtr createToState() const = 0;
        virtual bool isSameState(const StatePtr& from) const = 0;
        virtual bool canDoTransition(const StatePtr& from, const Event::Ptr& event) const = 0;
        virtual std::string fromStateToString() const = 0;
        virtual std::string toStateToString() const = 0;
        virtual std::string eventToString() const = 0;
        virtual bool isTransitionToExitState() const = 0;
        
        virtual std::string name() const = 0;
        
        virtual bool isEqual(const Ptr& transition) const
        {
            TransitionBase* tmp = transition.get();
            if( typeid(*(this)) == typeid(*(tmp)) )
            {
                return true;
            }
            return false;
        }
        
        virtual bool isAmbiguous(const Ptr& transition) const
        {
            if(transition->fromStateToString() == fromStateToString() &&
               transition->eventToString() == eventToString() &&
               transition->toStateToString() != toStateToString())
            {
                return true;
            }
            return false;
        }
    };
    
    
    template<typename TContext, typename from, typename to, typename event>
    class Transition : public TransitionBase<TContext>
    {
    public:
        typedef typename State<TContext>::Ptr StatePtr;
        typedef State<TContext> StateType;
        
        Transition()
        {}
        
        virtual StatePtr createToState() const
        {
            return std::make_shared<to>();
        }
        
        virtual bool isSameState(const StatePtr& fromState) const
        {
            StateType* tfrom = fromState.get();
            return typeid(*(tfrom)) == typeid(to);
        }
        
        virtual bool canDoTransition(const StatePtr& fromState, const Event::Ptr& triggerEvent) const
        {
            StateType* tfrom = fromState.get();
            Event* etrigger = triggerEvent.get();
            if( (typeid(*(tfrom)) == typeid(from)) && (typeid(*(etrigger)) == typeid(event)) )
            {
                return true;
            }
            return false;
        }
        
        virtual std::string fromStateToString() const
        {
            return Demangler::demangle(typeid(from).name());
        }
        
        virtual std::string toStateToString() const
        {
            return Demangler::demangle(typeid(to).name());
        }
        
        virtual std::string eventToString() const
        {
            return Demangler::demangle(typeid(event).name());
        }
        
        virtual bool isTransitionToExitState() const
        {
            if(std::is_base_of<ExitState<TContext>, to>::value)
            {
                return true;
            }
            return false;
        }
        
        virtual std::string name() const
        {
            return Demangler::demangle(typeid(*this).name());
        }
    };
    
    
    template<typename InitialState, typename TContext>
    class StateMachine : public StateMachineBase, public std::enable_shared_from_this<StateMachine<InitialState,TContext> >
    {
    public:
        typedef StateMachine<InitialState, TContext> SM;
        typedef std::shared_ptr<SM> Ptr;
        typedef typename State<TContext>::Ptr StatePtr;
        typedef typename TransitionBase<TContext>::Ptr TransitionBasePtr;
        typedef typename std::shared_ptr<TContext> ContextPtr;
        typedef TContext SMContext;
        
        StateMachine(const std::string& name)
        : mName(name)
        {}
        
        virtual ~StateMachine()
        {}
        
        const std::string& name() const
        {
            return mName;
        }
        
        ContextPtr context() const
        {
            return mContext;
        }
        
        void start()
        {
            mContext = std::make_shared<TContext>();
            mCurrentState = std::make_shared<InitialState>();
            mCurrentState->setContext(mContext);
            mCurrentState->setStateMachine(this->shared_from_this());
            
            doSetUp();
            mCurrentState->onEntry(statemachine::Event::Ptr());
            mCurrentState->run(statemachine::Event::Ptr());
        }
        
        StatePtr currentState() const
        {
            return mCurrentState;
        }
        
        void addEvent(const Event::Ptr& event)
        {
            mEvents.push(event);
        }
        
        void processEvent(const Event::Ptr& event = Event::Ptr())
        {
            if(event)
            {
                mEvents.push(event);
            }
            processEvents();
        }
        
        std::string generateDot()
        {
            mTransitions.clear();
            doSetUp();
            
            std::stringstream ss;
            
            ss << "digraph " << name() << " {" << std::endl;
            ss << "\tstart [shape=point];" << std::endl;
            
            std::set<std::string> exitStates;
            for(const auto& transition : mTransitions)
            {
                if(transition->isTransitionToExitState())
                {
                    exitStates.insert(transition->toStateToString());
                }
            }
            for(const auto& exitState : exitStates)
            {
                ss << "\t" << exitState << " [style=\"rounded\", shape=doubleoctagon];" << std::endl;
            }

            ss << "\tstart -> " << Demangler::demangle(typeid(InitialState).name()) << std::endl;
            for(const auto& transition : mTransitions)
            {
                ss << "\t" << transition->fromStateToString() << " -> " << transition->toStateToString() << " [label=\"" << transition->eventToString() << "\"];" << std::endl;
            }
            ss << "}" << std::endl;
            return ss.str();
        }
        
    protected:
        virtual void doSetUp() = 0;
        
        void processEvents()
        {
            if( !dynamic_cast<ExitState<TContext>*>(mCurrentState.get()) )
            {
                if(!mEvents.empty())
                {
                    while(!mEvents.empty())
                    {
                        auto event = mEvents.front();
                        mEvents.pop();
                        
                        bool transitionFound = false;
                        for(const auto& transition : mTransitions)
                        {
                            if(transition->canDoTransition(mCurrentState, event))
                            {
                                transitionFound = true;
                                
                                if(!transition->isSameState(mCurrentState))
                                {
                                    mCurrentState->onExit(event);
                                    mCurrentState = transition->createToState();
                                    mCurrentState->setContext(mContext);
                                    mCurrentState->setStateMachine(this->shared_from_this());
                                    mCurrentState->onEntry(event);
                                    
                                    if(dynamic_cast<ExitState<TContext>*>(mCurrentState.get()))
                                    {
                                        // an exit state will never call the run method
                                        return;
                                    }
                                }
                                
                                mCurrentState->run(event);
                                break;
                            }
                        }
                        if(!transitionFound)
                        {
                            noTransitionFound(event);
                        }
                    }
                }
                else
                {
                    mCurrentState->run(statemachine::Event::Ptr());
                }
            }
        }
        
        virtual void noTransitionFound(const Event::Ptr& event)
        {
            State<TContext>* current = mCurrentState.get();
            std::string stateType(typeid(*(current)).name());
            const Event* etrigger = event.get();
            std::string eventType(typeid(*(etrigger)).name());
            throw std::runtime_error("No transition found for state " + Demangler::demangle(stateType) + " with event " + Demangler::demangle(eventType));
        }
        
        void addTransition(const TransitionBasePtr& transition)
        {
            for(const auto& transition : mTransitions)
            {
                if(transition->isAmbiguous(transition))
                {
                    throw std::runtime_error(mName + ": Ambiguous transition found " + transition->name());
                }
            }
            mTransitions.push_back(transition);
        }
        
    private:
        typedef std::list<TransitionBasePtr> Transitions;
        typedef std::queue<Event::Ptr> Events;
        
        ContextPtr mContext;
        std::string mName;
        StatePtr mCurrentState;
        Transitions mTransitions;
        Events mEvents;
    };
    
#define SM_ADD_TRANSITION(from, to, event) \
addTransition(TransitionBasePtr(new statemachine::Transition<SMContext, from, to, event>))
    
}

#endif
