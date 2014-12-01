#ifndef GAMEENGINECOMMUNICATIONINTERFACEHPP
#define GAMEENGINECOMMUNICATIONINTERFACEHPP

#include<future>
#include<thread>
#include<exception>
#include<string>
#include<unistd.h> //For delay
#include "zmq.hpp"

#include "SOMException.hpp"
#include "portLocations.hpp"
#include "perceptOrActionMessage.pb.h"

/*
This class makes it easy to write a game for AI Arena by abstracting away all of the communication details so that the programmer can just call a few simple functions.
*/
class gameEngineCommunicationInterface
{
public:
/*
This function establishes the connections used to run the game interaction.
@param inputSizeOfAIPerceptionInBits:  The number of bits (starting at offset 0) that the agent should use (since the data is spaced out to the nearest byte)
@param inputSizeOfExpectedActionsInBits: The number of action bits that the game will accept from the agent (future versions may at some point allow this to change dynamically, but most AI architectures would have trouble supporting that).
@param inputActionTimeoutInterval:  The number of milliseconds that the game will wait before throwing an exception (it defaults to infinite wait)
@exceptions: This function can throw exceptions (especially if starting the connection to the AI times out)
*/
gameEngineCommunicationInterface(uint64_t inputSizeOfAIPerceptionsInBits, uint64_t inputSizeOfExpectedActionsInBits, int inputActionTimeoutInterval = -1);

/*
This function sends what the game decides the AI sees after its actions or initial starting state.  The class takes care of all of the details associated with sending AI perceptions and getting back the AI's actions.
@param inputAIPerceptions: The data to send to the agent for it to act on (must have more bits than the sizeOfExpectedActionsInBits.
@param inputReward: The reward that the game decides the AI is entitled to
@param inputEndGame: True if this is the last percept in the AI's current game and the next percept will correspond to a new game
@return: The actions submitted by the AI for the next round of the game
@exceptions: This function can throw some exceptions (especially if the connection to the other side times out or the AI chooses to terminate the game session).
*/
std::string sendPerceptionsAndGetActions(const std::string &inputAIPerceptions, uint64_t inputReward, bool inputEndGame = false);

/*
This function returns true if the AI has decided it would like to prematurely abort this game (with it being clear to all observers that it did) and start a new one.
@return: True if the AI has indicated a desire to start a new game prematurely
*/
bool AIWantsToRestartGame();

/*
This function returns true if the AI has indicated that it would like the game engine to shut down (terminating the process).
@return: True if the AI has indicated a desire to start a new game prematurely
*/
bool AIWantsToEndSession();


private:
bool aiWantsToRestartGameFlag;
bool aiWantsToEndSessionFlag;
std::string currentAction;
gameState currentGameState; //Start at the first percept of the new game, game over if the game is terminated, continue at any other time
uint64_t sizeOfAIPerceptionsInBits;
uint64_t sizeOfExpectedActionsInBits;
uint64_t sizeOfExpectedActionsInBytes;
std::unique_ptr<zmq::context_t> context;
std::unique_ptr<zmq::socket_t> perceptionsPublishingSocket;
std::unique_ptr<zmq::socket_t> actionReceptionSocket;

uint64_t perceptionSequenceCounter;

/*
This function publishes the given message on the perceptionsPublishingSocket and then tries to recieve a message from actionReceptionSocket.
@param inputMessage: The message to send
@param inputBlock: True if the function should block until the recv function times out
@return: The message received from actionReceptionSocket (or zero length on timeout)
@exceptions: This function can throw exceptions
*/
std::string sendMessageAndGetBackResponse(const std::string &inputMessage, bool inputBlock = true);

/*
This function deserializes the action message and updates the cached action values from it.
@param inputMessage: The message to extract the action bytes from
@exceptions: This function can throw exceptions, especially if the message in invalid
*/
void updateValuesFromMessage(const std::string &inputMessage);
};




















#endif
