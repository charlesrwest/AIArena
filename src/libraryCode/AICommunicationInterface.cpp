#include "AICommunicationInterface.hpp"

/*
This function establishes the connections used to run the AI/game interaction.
@exceptions: This function can throw exceptions (especially if starting the connection to the game times out)
*/
AICommunicationInterface::AICommunicationInterface()
{
//Remember size of AI perceptions in bits and size of expected actions in bits
perceptSequenceCounter = 0;

SOM_TRY
context.reset(new zmq::context_t);
SOM_CATCH("Error initializing ZMQ context\n")

//Initialize sockets associated with this object
SOM_TRY
actionPublishingSocket.reset(new zmq::socket_t(*context, ZMQ_PUB));
SOM_CATCH("Error initializing actions publishing socket\n")

//Now bind the socket
SOM_TRY
actionPublishingSocket->bind(("tcp://127.0.0.1:" + std::to_string(AIPORT)).c_str());
SOM_CATCH("Error binding socket\n")

SOM_TRY
perceptReceptionSocket.reset(new zmq::socket_t(*context, ZMQ_SUB));
SOM_CATCH("Error initializing percept subscription socket\n")

SOM_TRY
perceptReceptionSocket->connect(("tcp://localhost:" + std::to_string(GAMEPORT)).c_str());
SOM_CATCH("Error connecting to percept publisher\n")

SOM_TRY
perceptReceptionSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
SOM_CATCH("Error setting filter for percepts subscription\n")


//Get initial percept
SOM_TRY
updateCurrentPerceptCache();
SOM_CATCH("Error getting the first percept\n")
}

/*
This function retrieves the most recent perceptions.
@return: The perceptions associated with the current round of the game
@exceptions: This function can throw exceptions
*/
std::string AICommunicationInterface::getCurrentPerceptions()
{
return currentPercept;
}

/*
Get the reward associated with the last game round.
@return: The reward associated with the last game round
*/
uint64_t AICommunicationInterface::getCurrentReward()
{
return currentReward;
}

/*
Get the size of the perception in bits.
@return: The size of the perception in bits
@exceptions: This function can throw exceptions
*/
uint64_t AICommunicationInterface::getSizeOfPerceptionInBits()
{
return sizeOfPerceptionInBits;
}

/*
Get the size of an action specification in bits.
@return: The size of an action specification in bits.
@exceptions: This function can throw exceptions
*/
uint64_t AICommunicationInterface::getSizeOfActionSpecificationInBits()
{
return sizeOfExpectedActionInBits;
}

/*
This function sends what the AI decides to do.  The class takes care of all of the details associated with sending AI actions.  When the function returns, it is safe to retrieve percept and game related information.
@param inputAIActions: The data to send to the agent for it to act on (must have at least as many bits as sizeOfExpectedActionsInBits)
@param inputResetGame: Set this true to signal to the game that the AI would like to end the game prematurely
@param inputShutdownGame: Set this true to signal that the game engine should shut down

@exceptions: This function can throw some exceptions
*/
void AICommunicationInterface::sendActionsAndUpdatePerceptions(const std::string &inputAIActions, bool inputResetGame, bool inputShutdownGameEngine)
{
//Send the action
//Check the input
if(inputAIActions.size() != sizeOfExpectedActionInBytes)
{
throw SOMException("Error, action is not the expect size\n", INVALID_FUNCTION_INPUT, __FILE__, __LINE__);
}

//Create message to send
perceptOrActionMessage action;

action.set_action(inputAIActions);

if(inputResetGame)
{
action.set_game_state(GAME_OVER);
}

if(inputShutdownGameEngine)
{
action.set_terminate_game_session(true);
}
//Serialize action message
std::string serializedAction;
action.SerializeToString(&serializedAction);

//Send message
SOM_TRY
actionPublishingSocket->send(serializedAction.c_str(), serializedAction.size());
SOM_CATCH("Error sending message\n")

//Update from the next percept message if we didn't tell the game engine to shut down
if(!inputShutdownGameEngine)
{
updateCurrentPerceptCache();
}
}

/*
Update the catch of the current percept.
*/
void AICommunicationInterface::updateCurrentPerceptCache()
{
while(true) //Repeat until we get a valid update
{
//Get the serialized percept message
//Allocate a buffer for the reply message
std::unique_ptr<zmq::message_t> messageBuffer;
SOM_TRY
messageBuffer.reset(new zmq::message_t);  //This can throw a zmq::error_t
SOM_CATCH("Error constructing ZMQ message buffer\n")

SOM_TRY
if(perceptReceptionSocket->recv(messageBuffer.get(), 0) == false)
{
throw SOMException("Error, percept message retrieval timed out\n", TIME_OUT, __FILE__, __LINE__);
}
SOM_CATCH("Error receiving the reply message\n")

//Deserialize the percept
perceptOrActionMessage deserializedPerceptMessage;
deserializedPerceptMessage.ParseFromArray(messageBuffer->data(), messageBuffer->size());
if(!deserializedPerceptMessage.IsInitialized())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

if(!deserializedPerceptMessage.has_sequence_number())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

//Make sure the sequence number matches
if(perceptSequenceCounter != deserializedPerceptMessage.sequence_number())
{
continue;
}
perceptSequenceCounter++;

if(!deserializedPerceptMessage.has_percept())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

currentPercept = deserializedPerceptMessage.percept();

if(!deserializedPerceptMessage.has_reward())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

currentReward = deserializedPerceptMessage.reward();

if(!deserializedPerceptMessage.has_size_of_percept_in_bits())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

sizeOfPerceptionInBits = deserializedPerceptMessage.size_of_percept_in_bits();


if(!deserializedPerceptMessage.has_size_of_expected_action())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

sizeOfExpectedActionInBits = deserializedPerceptMessage.size_of_expected_action();
sizeOfExpectedActionInBytes = (sizeOfExpectedActionInBits + 7)/8; //Round up

if(!deserializedPerceptMessage.has_game_state())
{
//Message can't be read, so throw an exception
throw SOMException("Error, percept message is invalid\n", INCORRECT_SERVER_RESPONSE, __FILE__, __LINE__);
}

currentGameState = deserializedPerceptMessage.game_state();
return; //Everything was updated successfully, so exit
}
}

