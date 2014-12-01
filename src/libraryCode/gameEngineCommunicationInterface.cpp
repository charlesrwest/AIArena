#include "gameEngineCommunicationInterface.hpp"

/*
This function establishes the connections used to run the game interaction.
@param inputSizeOfAIPerceptionInBits:  The number of bits (starting at offset 0) that the agent should use (since the data is spaced out to the nearest byte)
@param inputSizeOfExpectedActionsInBits: The number of action bits that the game will accept from the agent (future versions may at some point allow this to change dynamically, but most AI architectures would have trouble supporting that).
@param inputActionTimeoutInterval:  The number of milliseconds that the game will wait before throwing an exception (it defaults to infinite wait)
@exceptions: This function can throw exceptions (especially if starting the connection to the AI times out)
*/
gameEngineCommunicationInterface::gameEngineCommunicationInterface(uint64_t inputSizeOfAIPerceptionsInBits, uint64_t inputSizeOfExpectedActionsInBits, int inputActionTimeoutInterval)
{
//Remember size of AI perceptions in bits and size of expected actions in bits
sizeOfAIPerceptionsInBits = inputSizeOfAIPerceptionsInBits;
sizeOfExpectedActionsInBits = inputSizeOfExpectedActionsInBits;
sizeOfExpectedActionsInBytes = ((sizeOfExpectedActionsInBits+7)/8);
perceptionSequenceCounter = 0;
currentGameState = GAME_START;
aiWantsToRestartGameFlag = false;
aiWantsToEndSessionFlag = false;

SOM_TRY
context.reset(new zmq::context_t);
SOM_CATCH("Error initializing ZMQ context\n")

//Initialize sockets associated with this object
SOM_TRY
perceptionsPublishingSocket.reset(new zmq::socket_t(*context, ZMQ_PUB));
SOM_CATCH("Error initializing perceptions publishing socket\n")

//Now bind the socket
SOM_TRY
perceptionsPublishingSocket->bind(("tcp://127.0.0.1:" + std::to_string(GAMEPORT)).c_str());
SOM_CATCH("Error binding socket\n")

SOM_TRY
actionReceptionSocket.reset(new zmq::socket_t(*context, ZMQ_SUB));
SOM_CATCH("Error initializing actions subscription socket\n")

SOM_TRY
actionReceptionSocket->connect(("tcp://localhost:" + std::to_string(AIPORT)).c_str());
SOM_CATCH("Error connecting to action publisher\n")

SOM_TRY
actionReceptionSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
SOM_CATCH("Error setting filter for actions subscription\n")

if(inputActionTimeoutInterval >= 0)
{
SOM_TRY
actionReceptionSocket->setsockopt(ZMQ_RCVTIMEO, &inputActionTimeoutInterval, sizeof(inputActionTimeoutInterval));
SOM_CATCH("Error setting timeout interval for action reception socket\n")
}

}

/*
This function sends what the game decides the AI sees after its actions or initial starting state.  The class takes care of all of the details associated with sending AI perceptions and getting back the AI's actions.
@param inputAIPerceptions: The data to send to the agent for it to act on (must have more bits than the sizeOfExpectedActionsInBits.
@param inputReward: The reward that the game decides the AI is entitled to
@return: The actions submitted by the AI for the next round of the game
@exceptions: This function can throw some exceptions (especially if the connection to the other side times out or the AI chooses to terminate the game session).
*/
std::string gameEngineCommunicationInterface::sendPerceptionsAndGetActions(const std::string &inputAIPerceptions, uint64_t inputReward, bool inputEndGame)
{
//Create serialized version of the perception
perceptOrActionMessage percept;

percept.set_size_of_percept_in_bits(sizeOfAIPerceptionsInBits);
percept.set_size_of_expected_action(sizeOfExpectedActionsInBits);
percept.set_percept(inputAIPerceptions);
percept.set_reward(inputReward);
percept.set_sequence_number(perceptionSequenceCounter);

percept.set_game_state(currentGameState);

if(currentGameState == GAME_START)  //Set the game state for the next percept
{
currentGameState = GAME_CONTINUE;
}

//If the game has end, set this percept to GAME_OVER and make the next GAME_START
if(inputEndGame) 
{
percept.set_game_state(GAME_OVER);
currentGameState = GAME_START;
aiWantsToRestartGameFlag = false;
}

//Serialize
std::string serializedPercept;
percept.SerializeToString(&serializedPercept);

//Check if this is the initial perception, so it can be sent multiple times until the AI on the other side picks up
if(perceptionSequenceCounter == 0)
{
std::string replyMessage;

for(int i=0; i<100; i++) //Publish the initial perception 100 times before giving up
{
//Send percept and try to get reply
SOM_TRY
replyMessage = sendMessageAndGetBackResponse(serializedPercept, false);
SOM_CATCH("Error sending percept/getting reply\n")

if(replyMessage.size() == 0)
{
sleep(1); //Delay 1 second before sending the percept again (TODO: decrease delay duration)
continue; //Message timed out, so try again
}

SOM_TRY
updateValuesFromMessage(replyMessage);
SOM_CATCH("Error getting action from message\n")
perceptionSequenceCounter++;
return currentAction;
}

//Never got an action, so the other side probably had a problem
throw SOMException("Error, action message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

std::string replyMessage;

//Send percept and try to get reply
SOM_TRY
replyMessage = sendMessageAndGetBackResponse(serializedPercept);
SOM_CATCH("Error sending percept/getting reply\n")

if(replyMessage.size() == 0)
{
throw SOMException("Error, action message timed out\n", TIME_OUT, __FILE__, __LINE__);
}

SOM_TRY
updateValuesFromMessage(replyMessage);
SOM_CATCH("Error getting action from message\n")

perceptionSequenceCounter++;
return currentAction;
}



/*
This function publishes the given message on the perceptionsPublishingSocket and then tries to recieve a message from actionReceptionSocket.
@param inputMessage: The message to send
@param inputBlock: True if the function should block until the recv function times out
@return: The message received from actionReceptionSocket (or zero length on timeout)
@exceptions: This function can throw exceptions
*/
std::string gameEngineCommunicationInterface::sendMessageAndGetBackResponse(const std::string &inputMessage, bool inputBlock)
{
//Send message
SOM_TRY
perceptionsPublishingSocket->send(inputMessage.c_str(), inputMessage.size());
SOM_CATCH("Error sending message\n")

//Allocate a buffer for the reply message
std::unique_ptr<zmq::message_t> messageBuffer;
SOM_TRY
messageBuffer.reset(new zmq::message_t);  //This can throw a zmq::error_t
SOM_CATCH("Error constructing ZMQ message buffer\n")


//Receive reply
int flags = 0;
if(!inputBlock)
{
flags = ZMQ_DONTWAIT;  //Set nonblocking if we are not suppose to wait
}

SOM_TRY
if(actionReceptionSocket->recv(messageBuffer.get(), flags) == false)
{
return std::string();
}
SOM_CATCH("Error receiving the reply message\n")

return std::string((const char *) messageBuffer->data(), messageBuffer->size());
}

/*
This function deserializes the action message and updates the cached action values from it.
@param inputMessage: The message to extract the action bytes from
@exceptions: This function can throw exceptions, especially if the message in invalid
*/
void gameEngineCommunicationInterface::updateValuesFromMessage(const std::string &inputMessage)
{
perceptOrActionMessage deserializedActionMessage;
deserializedActionMessage.ParseFromString(inputMessage);
if(!deserializedActionMessage.IsInitialized())
{
//Message can't be read, so throw an exception
throw SOMException("Error, action message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

if(!deserializedActionMessage.has_action())
{
//Message can't be read, so throw an exception
throw SOMException("Error, action message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

currentAction = deserializedActionMessage.action();
if(currentAction.size() < sizeOfExpectedActionsInBytes)
{
//Message can't be read, so throw an exception
throw SOMException("Error, action message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

//TODO: Need to refactor this function and have it cache values
//Check if the agent wants to reset the game
if(deserializedActionMessage.has_game_state())
{
aiWantsToRestartGameFlag = true;
}

if(deserializedActionMessage.has_terminate_game_session())
{
aiWantsToEndSessionFlag = deserializedActionMessage.terminate_game_session();
}

}

/*
This function returns true if the AI has decided it would like to prematurely abort this game (with it being clear to all observers that it did) and start a new one.
@return: True if the AI has indicated a desire to start a new game prematurely
*/
bool gameEngineCommunicationInterface::AIWantsToRestartGame()
{
return aiWantsToRestartGameFlag;
}

/*
This function returns true if the AI has indicated that it would like the game engine to shut down (terminating the process).
@return: True if the AI has indicated a desire to start a new game prematurely
*/
bool gameEngineCommunicationInterface::AIWantsToEndSession()
{
return aiWantsToEndSessionFlag;
}


