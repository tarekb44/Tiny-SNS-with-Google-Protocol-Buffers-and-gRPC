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
        std::cout << "connection failed: " << ire.grpc_status.error_message() << std::endl;
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
      ire = UnFollow(arg);
    }

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

  grpc::Status status = stub_->List(&context, request, &list_reply);

  ire.grpc_status = status;

  if (status.ok()) {
      for (const auto& user : list_reply.all_users()) {
          ire.all_users.push_back(user);
      }

      for (const auto& follower : list_reply.followers()) {
          ire.followers.push_back(follower);
      }

      ire.comm_status = SUCCESS;
  } else {
      ire.comm_status = FAILURE_UNKNOWN;
  }

  return ire;
}

// Follow Command        
IReply Client::Follow(const std::string& username2) {

  IReply ire; 
    
  Request request;
  request.set_username(this->username);
  request.add_arguments(username2);

  ClientContext context;
  Reply reply;

  grpc::Status status = stub_->Follow(&context, request, &reply);

  ire.grpc_status = status;

  if (status.ok()) {
      if (reply.msg() == "you have already joined") {
          ire.comm_status = FAILURE_ALREADY_EXISTS;
      } else if (reply.msg() == "Follow Successful") {
          ire.comm_status = SUCCESS;
      } else if (reply.msg() == "Invalid username") {
          ire.comm_status = FAILURE_INVALID_USERNAME;
      } else {
          ire.comm_status = FAILURE_UNKNOWN;
      }
  } else {
      ire.comm_status = FAILURE_UNKNOWN;
  }

  return ire;
}

// UNFollow Command  
IReply Client::UnFollow(const std::string& username2) {

  IReply ire;

  Request request;
  request.set_username(this->username);
  request.add_arguments(username2);

  ClientContext context;
  Reply reply;

  grpc::Status status = stub_->UnFollow(&context, request, &reply);

  ire.grpc_status = status;

  if (status.ok()) {
    if (reply.msg() == "You are not a follower") {
      ire.comm_status = FAILURE_NOT_A_FOLLOWER;
    } else if (reply.msg() == "UnFollow Successful") {
      ire.comm_status = SUCCESS;
    } else {
      ire.comm_status = FAILURE_UNKNOWN;
    }
  } else {
    ire.comm_status = FAILURE_UNKNOWN;
  }

  return ire;
}

IReply Client::Login() {
    IReply ire;
    Request request;
    request.set_username(username);

    Reply reply;
    ClientContext context;

    // Call the Login RPC
    grpc::Status status = stub_->Login(&context, request, &reply);

    ire.grpc_status = status;

    if (status.ok()) {
        ire.comm_status = SUCCESS;
        if (reply.msg() == "you have already joined") {
            ire.comm_status = FAILURE_ALREADY_EXISTS;
        } else {
            ire.comm_status = SUCCESS;
        }
    } else if (status.error_code() == grpc::StatusCode::ALREADY_EXISTS) {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
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
  
    /***
    YOUR CODE HERE
    ***/

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
      
  std::cout << "Logging Initialized. Client starting...";
  
  Client myc(hostname, username, port);
  
  myc.run();
  
  return 0;
}
