#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <csignal>
#include <grpc++/grpc++.h>
#include "client.h"

#include "sns.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce662::Message;
using csce662::ListReply;
using csce662::Request;
using csce662::Reply;
using csce662::SNSService;

void sig_ignore(int sig) {
  std::cout << "Signal caught " + sig;
}

Message MakeMessage(const std::string& username, const std::string& msg) {
    Message m;
    m.set_username(username);
    m.set_msg(msg);
    google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
    timestamp->set_seconds(time(NULL));
    timestamp->set_nanos(0);
    m.set_allocated_timestamp(timestamp);
    return m;
}


class Client : public IClient
{
public:
  Client(const std::string& hname,
	 const std::string& uname,
	 const std::string& p)
    :hostname(hname), username(uname), port(p) {}

  
protected:
  virtual int connectTo();
  virtual IReply processCommand(std::string& input);
  virtual void processTimeline();

private:
  std::string hostname;
  std::string username;
  std::string port;
  
  // You can have an instance of the client stub
  // as a member variable.
  std::unique_ptr<SNSService::Stub> stub_;
  
  IReply Login();
  IReply List();
  IReply Follow(const std::string &username);
  IReply UnFollow(const std::string &username);
  void   Timeline(const std::string &username);
};

int Client::connectTo() {
    std::string connection_info = hostname + ":" + port;
    stub_ = SNSService::NewStub(
        grpc::CreateChannel(connection_info, grpc::InsecureChannelCredentials()));

    IReply ire = Login();

    if (ire.grpc_status.ok() && ire.comm_status == SUCCESS) {
        return 1;
    } else {
        // std::cout << "connection failed: " << ire.grpc_status.error_message() << std::endl;
        return -1;
    }
}

IReply Client::processCommand(std::string& input)
{
  // ------------------------------------------------------------
  // GUIDE 1:
  // In this function, you are supposed to parse the given input
  // command and create your own message so that you call an 
  // appropriate service method. The input command will be one
  // of the followings:
  //
  // FOLLOW <username>
  // UNFOLLOW <username>
  // LIST
  // TIMELINE
  // ------------------------------------------------------------
  
  // ------------------------------------------------------------
  // GUIDE 2:
  // Then, you should create a variable of IReply structure
  // provided by the client.h and initialize it according to
  // the result. Finally you can finish this function by returning
  // the IReply.
  // ------------------------------------------------------------
  
  
  // ------------------------------------------------------------
  // HINT: How to set the IReply?
  // Suppose you have "FOLLOW" service method for FOLLOW command,
  // IReply can be set as follow:
  // 
  //     // some codes for creating/initializing parameters for
  //     // service method
  //     IReply ire;
  //     grpc::Status status = stub_->FOLLOW(&context, /* some parameters */);
  //     ire.grpc_status = status;
  //     if (status.ok()) {
  //         ire.comm_status = SUCCESS;
  //     } else {
  //         ire.comm_status = FAILURE_NOT_EXISTS;
  //     }
  //     
  //      return ire;
  // 
  // IMPORTANT: 
  // For the command "LIST", you should set both "all_users" and 
  // "following_users" member variable of IReply.
  // ------------------------------------------------------------

    IReply ire;

    // std::cout << "Cmd" << input << std::endl;

  std::string command, arg;
  size_t space_pos = input.find(' ');
  if (space_pos != std::string::npos)
  {
    // extract the command line args (e.g. FOLLOW <username>)
    command = input.substr(0, space_pos);
    arg = input.substr(space_pos + 1);
  } 
  else 
  {
    command = input;
    arg = "";
  } 

  if (command == "FOLLOW")
  { 
    if (arg != "")
      ire = Follow(arg);
  } 
  else if (command == "LIST") 
  {
    ire = List();
  }
  else if (command == "UNFOLLOW")
  {
    if (arg!= "")
      ire = UnFollow(arg);
  }
  else if (command == "TIMELINE")
  {
    processTimeline();
    ire.comm_status = SUCCESS;
  }
  else 
    ire.comm_status = FAILURE_UNKNOWN;

  return ire;
}


void Client::processTimeline()
{
    Timeline(username);
}

// List Command
IReply Client::List() {
  IReply ire;
  Request request;

  request.set_username(this->username);

  ListReply list_reply;
  ClientContext context;

  // call the stub from the server for list
  grpc::Status status = stub_->List(&context, request, &list_reply);

  // check this is a valdi response
  ire.grpc_status = status;

  if (status.ok())
  {
    // populate the users that has been retrieved from the server
    for (const auto& user : list_reply.all_users())
      ire.all_users.push_back(user);

    // populate the followers that has been retrieved from the server
    for (const auto& follower : list_reply.followers()) 
      ire.followers.push_back(follower);

    ire.comm_status = SUCCESS;
  } 
  else
    ire.comm_status = FAILURE_UNKNOWN;

  return ire;
}

// Follow Command        
IReply Client::Follow(const std::string& username2) {
  IReply ire; 
  Request request;
  ClientContext context;
  Reply reply;

  // use set_username and add_arguments, which are already by grpc compiler
  request.set_username(this->username);
  request.add_arguments(username2);

  // ask the server for a follow function
  grpc::Status status = stub_->Follow(&context, request, &reply);

  ire.grpc_status = status;

  if (status.ok())
  {
    if (reply.msg() == "you have already joined")
      ire.comm_status = FAILURE_ALREADY_EXISTS;
    else if (reply.msg() == "Follow Successful")
      ire.comm_status = SUCCESS;
    else if (reply.msg() == "Invalid username")
      ire.comm_status = FAILURE_INVALID_USERNAME;
    else
      ire.comm_status = FAILURE_UNKNOWN;
  }

  return ire;
}

// UNFollow Command  
IReply Client::UnFollow(const std::string& username2) {
  IReply ire;
  Request request;
  ClientContext context;
  Reply reply;

  request.set_username(this->username);
  request.add_arguments(username2);

  // ask the server stub for unfollow command
  grpc::Status status = stub_->UnFollow(&context, request, &reply);

  ire.grpc_status = status;

  if (status.ok())
  {
    if (reply.msg() == "you are not a follower")
      ire.comm_status = FAILURE_ALREADY_EXISTS;
    else if (reply.msg() == "UnFollow Successful")
      ire.comm_status = SUCCESS;
    else if (reply.msg() == "Invalid username")
      ire.comm_status = FAILURE_INVALID_USERNAME; 
    else
      ire.comm_status = FAILURE_UNKNOWN;
  }

  return ire;
}

IReply Client::Login() {
  IReply ire;
  Request request;
  Reply reply;
  ClientContext context;
  
  request.set_username(username);

  // Call the Login RPC from stub
  grpc::Status status = stub_->Login(&context, request, &reply);

  ire.grpc_status = status;

  if (status.ok())
  {
    if (reply.msg() == "you have already joined")
      ire.comm_status = FAILURE_ALREADY_EXISTS;
    else
      ire.comm_status = SUCCESS;
  }

  return ire;
}


// Timeline Command
void Client::Timeline(const std::string& username) {
  // ------------------------------------------------------------
  // In this function, you are supposed to get into timeline mode.
  // You may need to call a service method to communicate with
  // the server. Use getPostMessage/displayPostMessage functions 
  // in client.cc file for both getting and displaying messages 
  // in timeline mode.
  // ------------------------------------------------------------

  // ------------------------------------------------------------
  // IMPORTANT NOTICE:
  //
  // Once a user enter to timeline mode , there is no way
  // to command mode. You don't have to worry about this situation,
  // and you can terminate the client program by pressing
  // CTRL-C (SIGINT)
  // ------------------------------------------------------------
  
  ClientContext context;
  std::shared_ptr<grpc::ClientReaderWriter<Message, Message>> stream(
    stub_->Timeline(&context));

  //thread to read messages from the server
  std::thread reader([stream, this]() {
    Message server_msg;
    // get the message back from the server
    // infinite loop to read back from derver
    while (stream->Read(&server_msg)) {
      std::time_t time = static_cast<std::time_t>(server_msg.timestamp().seconds());
      displayPostMessage(server_msg.username(), server_msg.msg(), time);
    }
  });

  // we need a way to read and write to the server at the same time, so we need to employ some threads
  // thread to send messages to the server
  std::thread writer([stream, this]() {
    // infinite loop to send the msgs
    while (true) {
      std::string message = getPostMessage();
      // use the message struct to define the username (which should be in the server db), 
      //and the message we want to senf
      Message msg = MakeMessage(this->username, message);
      stream->Write(msg);
    }
  });

  writer.join();
  reader.join();

  // Finish the stream
  //stream->WritesDone();
  //grpc::Status status = stream->Finish();
  //if (!status.ok()) {
  //    std::cerr << "Timeline rpc failed." << std::endl;
  //}
}



//////////////////////////////////////////////
// Main Function
/////////////////////////////////////////////
int main(int argc, char** argv) {

  std::string hostname = "localhost";
  std::string username = "default";
  std::string port = "3010";
    
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
    switch(opt) {
    case 'h':
      hostname = optarg;break;
    case 'u':
      username = optarg;break;
    case 'p':
      port = optarg;break;
    default:
      std::cout << "Invalid Command Line Argument\n";
    }
  }
      
  // std::cout << "Logging Initialized. Client starting...";
  
  Client myc(hostname, username, port);
  
  myc.run();
  
  return 0;
}
