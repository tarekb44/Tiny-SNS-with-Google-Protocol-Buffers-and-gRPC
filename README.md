# Tiny SNS with Google Protocol Buffers and gRPC

## Overview
Tiny Social Networking Service (SNS) using Google Protocol Buffers and gRPC. The Tiny SNS provides basic functionalities similar to posting and receiving status updates on social media platforms like Facebook or Twitter. There is a server and client that allow users to follow others, post updates, and manage timelines.

## Features
The Tiny SNS service supports the following functionalities:

1. **User Modes**: Each user can operate in two modes:
   - **Command Mode**: This is the default mode when the client starts. Users can execute various commands such as `FOLLOW`, `UNFOLLOW`, `LIST`, and `TIMELINE`.
   - **Timeline Mode**: Allows users to post updates to their timelines and view posts from users they follow.

2. **LOGIN**: 
   - Users can log in to the SNS using a unique username. Duplicate logins are not allowed.

3. **FOLLOW**: 
   - Users can follow other users using the command `FOLLOW <username>`.
   - Once following, users can view new posts from the followed user in real-time.

4. **UNFOLLOW**: 
   - Users can unfollow others using the command `UNFOLLOW <username>`.
   - This removes the user from the followed user's timeline.

5. **LIST**:
   - Retrieves the list of all existing users and the list of followers for the current user.
   - The command provides a way to view the user network.

6. **TIMELINE**:
   - Users can post updates to their timeline.
   - The command switches a user to timeline mode, where they can post updates and view posts from others they follow.
   - In timeline mode, the user immediately sees the last 20 posts from users they follow.
   - Uses synchronous streaming to ensure real-time updates on the user's timeline.

7. **Server Persistency**:
   - All timelines are stored persistently on the server side.
   - Posts are saved in files with the format:
     ```
     T 2009-06-01 00:00:00
     U http://twitter.com/testuser
     W Post content
     ```

## Communication
All client-server communications use Google Protocol Buffers v3 and gRPC.

## Getting Started

### Prerequisites
- Google Protocol Buffers v3
- gRPC
- C++ compiler
- Provided virtual machine environment where Protocol Buffers and gRPC are pre-installed

### Running the System
To start the server and client:

1. Start the server:
   ```bash
   ./tsd -p <port_number>

1. Start the client:
./tsc -h <host_name> -p <port_number> -u <username>

