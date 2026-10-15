
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

// Linux USB Serial 통신
class SerialPort {
public:
    explicit SerialPort(const std::string& device) {
        fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd_ < 0)
            throw std::runtime_error("open: " + std::string(std::strerror(errno)));

        termios tty{};
        if (tcgetattr(fd_, &tty) != 0)
            fail("tcgetattr");

        cfmakeraw(&tty);
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
        tty.c_cflag |= CS8;

        if (tcsetattr(fd_, TCSANOW, &tty) != 0)
            fail("tcsetattr");
    }

    ~SerialPort() {
        if (fd_ >= 0) ::close(fd_);
    }

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    void send(const std::string& json) {
        const std::string message = json + "\n";
        std::size_t sent = 0;

        while (sent < message.size()) {
            ssize_t n = ::write(
                fd_, message.data() + sent,
                message.size() - sent
            );

            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) fail("write");

            sent += static_cast<std::size_t>(n);
        }

        if (tcdrain(fd_) != 0)
            fail("tcdrain");
    }

private:
    int fd_ = -1;

    [[noreturn]] static void fail(const char* action) {
        throw std::runtime_error(
            std::string(action) + ": " + std::strerror(errno)
        );
    }
};

static void usage(const char* program) {
    std::cerr
        << "Usage:\n"
        << "  " << program
        << " --dry-run stop|home|angle X Y|move X Y\n"
        << "  " << program
        << " /dev/ttyUSB0 stop|home|angle X Y|move X Y\n"
        << "  angle X=-180..180 Y=-30..90 (degrees)\n"
        << "  move X,Y=-1..1 (200 ms then stop)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const bool dry_run = std::string(argv[1]) == "--dry-run";
    const std::string action = argv[2];
    std::string command;

    try {
        if (action == "stop" && argc == 3) {
            command = R"({"T":135})";
        }
        else if (action == "home" && argc == 3) {
            command = R"({"T":133,"X":0,"Y":0,"SPD":100,"ACC":50})";
        }
        else if ((action == "angle" || action == "move") && argc == 5) {
            int x = std::stoi(argv[3]);
            int y = std::stoi(argv[4]);

            std::ostringstream os;

            if (action == "angle") {
                if (x < -180 || x > 180 || y < -30 || y > 90)
                    throw std::invalid_argument("angle range exceeded");

                os << "{\"T\":133,\"X\":" << x
                   << ",\"Y\":" << y
                   << ",\"SPD\":100,\"ACC\":50}";
            }
            else {
                if (x < -1 || x > 1 || y < -1 || y > 1)
                    throw std::invalid_argument("move direction must be -1, 0, or 1");

                os << "{\"T\":141,\"X\":" << x
                   << ",\"Y\":" << y
                   << ",\"SPD\":100}";
            }

            command = os.str();
        }
        else {
            usage(argv[0]);
            return 1;
        }

        // 장비 없이 명령만 출력
        if (dry_run) {
            std::cout << "[DRY RUN] " << command << '\n';
            if (action == "move")
                std::cout << "[DRY RUN] {\"T\":135} (after 200 ms)\n";
            return 0;
        }

        // 실제 Serial 명령 송신
        SerialPort serial(argv[1]);

        std::cout << "SEND " << command << '\n';
        serial.send(command);

        // 수동 이동은 200ms 후 정지 명령 송신
        if (action == "move") {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "SEND {\"T\":135}\n";
            serial.send(R"({"T":135})");
        }

        std::cout << "Sent (device operation not verified).\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
