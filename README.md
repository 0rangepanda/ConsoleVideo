# A Console-Based Video Chat Application

## Where's the fun?
1. Show image by gray-level graph on a console.
2. Transfer only updated pixels to reduce bandwidth required.



## Brainstorm
1. Server listen to a certain port (i.e. 3478 for stun server)
2. When user login, send a binding request to 3478.
3. When there comes a binding request, check authority firstly, then allocate a port to the user (otherwise deny that user).
4. There should be a database to store user's username (should be unique for making a call) and password.
5. When one calls another, send a calling request to the server. Server checks if that user is online. If it is, server will set up a bridge between these two ports and relay data-stream for them (maybe creating a new thread for each session).
