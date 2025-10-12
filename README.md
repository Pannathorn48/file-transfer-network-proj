# UDP File Transfer System

A reliable UDP-based client-server file transfer application implementing custom protocols for robust data transmission over unreliable networks.

## 🚀 Features

- **Reliable UDP Communication**: Custom implementation of reliability over UDP with checksums and acknowledgments
- **File Segmentation**: Automatic file chunking for efficient network transmission
- **Error Detection & Recovery**: Checksum validation with automatic retransmission on corruption
- **Selective Repeat Protocol**: Windowed transmission with selective acknowledgments for optimal performance  
- **Handshake Protocol**: Connection establishment with metadata exchange
- **Interactive Client Interface**: Simple command-line interface for file requests
- **Comprehensive Error Handling**: Detailed error reporting for network and file system issues

## ️ Requirements

- **Compiler**: GCC with C99 support
- **Network**: UDP socket support
- **Build Tools**: Make

## ⚙️ Installation & Setup

### Method 1: Using Environment Profile (Recommended)

1. **Activate the project environment:**
   ```bash
   source ./profile/activate
   ```
   This configures shell functions and environment variables for easy project management.

2. **Build and run the server:**
   ```bash
   server <port>
   ```

3. **Build and run the client (in another terminal):**
   ```bash
   source ./profile/activate  # If not already activated
   client <server_ip> <port>
   ```

4. **Deactivate when finished:**
   ```bash
   deactivate
   ```

### Method 2: Manual Build

1. **Build the entire project:**
   ```bash
   make build
   ```

2. **Run the server:**
   ```bash
   ./bin/server <port>
   ```

3. **Run the client:**
   ```bash
   ./bin/client <server_ip> <port>
   ```

## 🎯 Usage Examples

### Starting the Server
```bash
# Using environment profile
source ./profile/activate
server 8080

# Or manually
make server
./bin/server 8080
```

### Connecting with Client
```bash
# Using environment profile  
source ./profile/activate
client 127.0.0.1 8080

# Or manually
make client
./bin/client 127.0.0.1 8080
```

### File Transfer Session
```
Enter filename to request (or 'quit' to exit): document.txt
[File transfer begins...]
File received successfully: document.txt

Enter filename to request (or 'quit' to exit): quit
```

## 🔧 Build Targets

| Command | Description |
|---------|-------------|
| `make build` | Build both client and server |
| `make server` | Build server only |
| `make client` | Build client only |
| `make test` | Build and run checksum tests |
| `make clean` | Remove all build artifacts |
| `make clear` | Alias for clean |

## 📡 Protocol Details

### Message Structure
- **Checksum**: Error detection using custom checksum algorithm
- **Flags**: 32-bit header with metadata, ACK/NACK, sequence numbers, and status codes
- **Data**: Variable-length payload (up to 1024 bytes per packet)

### Connection Flow
1. **Handshake**: Client initiates META packet, server responds with META-ACK
2. **File Request**: Client sends filename in data payload  
3. **File Transfer**: Server segments file and transmits with sequence numbers
4. **Acknowledgment**: Client sends ACK/NACK for each received packet
5. **Completion**: Server sends FIN packet when transfer complete

### Reliability Features
- **Checksum Validation**: All packets validated on receipt
- **Automatic Retransmission**: Corrupted or lost packets retransmitted  
- **Windowed Transmission**: Selective repeat with configurable window size
- **Timeout Handling**: Configurable timeouts with exponential backoff

## 🧪 Testing

Run the included test suite:
```bash
make test
```

This executes checksum validation tests to ensure data integrity functions work correctly.

##  Configuration

### Environment Variables
Set in `profile/activate`:
- `NETWORK_ACTIVATE`: Environment activation flag
- `_OLD_PATH`: Path backup for deactivation

### Protocol Constants
Defined in `headers/message.h`:
- `BUFFER_SIZE`: 1024 bytes
- `WINDOW_SIZE`: 5 packets  
- `TIMEOUT_MSEC`: 3000ms
- `TEST_SENDER_PACKET_LOSS_PERCENTAGE` : 1\%
- `TEST_SENDER_PACKET_CORRUPTION_PERCENTAGE` : 10\%
- `TEST_SENDER_DUPLICATE_PACKET_PERCENTAGE` : 1\%
- `TEST_RECEIVER_PACKET_LOSS_PERCENTAGE` : 1\%