#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int64_multi_array.hpp"
#include "bfc_msgs/msg/button.hpp"

#include "RoboCupGameControlData.h"

#define IN_PORT 3838
#define OUT_PORT 3939
#define MAX_LENGTH 4096

using std::placeholders::_1;
using namespace std::chrono_literals;

// read data
struct sockaddr_in local_addr;
static sockaddr_in source_addr;

// return data
struct sockaddr_in other;
char message[MAX_LENGTH];

class game_controller : public rclcpp::Node
{
public:
    game_controller() : Node("game_controller",rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        this->declare_parameter("Player", 0);
        this->declare_parameter("Team", 0);
        this->declare_parameter("Server", "");
        Player = this->get_parameter("Player").as_int();
        Team = this->get_parameter("Team").as_int();
        server_string = this->get_parameter("Server").as_string();
        Server = server_string.c_str();

        Publisher_ = this->create_publisher<std_msgs::msg::Int64MultiArray>(
            "game_controller", 10);

        timer_ = this->create_wall_timer(
            std::chrono::microseconds(50000),
            std::bind(&game_controller::publishData, this));
    }

    void readButton( const bfc_msgs::msg::Button::SharedPtr msg )
    {
        Penalise = msg->kill;
    }

    void publishData()
    {
        readGameControllerData();
        returnGameControllerData();
    }

    void mexExit()
    {
        if (sock_fd > 0)
        {
            close(sock_fd);
        }
        if (sock_rt > 0)
        {
            close(sock_rt);
        }
    }

    bool initRead()
    {
        init_read = false;
        // TODO: figure out lua error throw method
        if (!init_read)
        {
            sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
            // sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

            // if (sock_fd < 0) //printf("Could not open datagram socket\n");
            // struct sockaddr_in local_addr;
            bzero((char *)&local_addr, sizeof(local_addr));
            local_addr.sin_family = AF_INET;
            local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            local_addr.sin_port = htons(IN_PORT);
            if (bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
            {
                printf("Could not bind to port\n");
            }

            broadcast = 1;
            if ((setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))))
            {
                perror("setsockopt");
            }

            // Nonblocking receive:
            /*flags  = fcntl(sock_fd, F_GETFL, 0);
            if (flags == -1) { flags = 0; }
            if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) { }*/
            // TODO: set lua on close? is it possible
            init_read = true;
        }

        return init_read;
    }

    void readGameControllerData()
    {
        if (!initRead())
        {
            initRead();
        }

        // Process incoming game controller messages:
        // static sockaddr_in source_addr;
        socklen_t source_addr_len = sizeof(source_addr);

        len = recvfrom(sock_fd, data, MAX_LENGTH, 0, (struct sockaddr *)&source_addr, &source_addr_len);
        while (len > 0)
        {
            // printf("Packet: %d bytes\n", len);
            if (memcmp(data, GAMECONTROLLER_STRUCT_HEADER, sizeof(GAMECONTROLLER_STRUCT_HEADER) - 1) == 0)
            {
                memcpy(&gameControlData, data, sizeof(RoboCupGameControlData));
                // nGameControlData++; //printf("Game control: %d received.\n", nGameControlData);

                if (len != 116 && len != 0)
                {
                    // if (len >= 158) {
                    // Robocup GameController Data
                    State = gameControlData.state;
                    FirstHalf = gameControlData.firstHalf;
                    Version = gameControlData.version;
                    PacketNumber = gameControlData.packetNumber;
                    PlayerTeam = gameControlData.playersPerTeam;
                    // GameTipe	= gameControlData.gameType;
                    KickOff = gameControlData.kickOffTeam;
                    SecondaryState = gameControlData.secondaryState;
                    DropTeam = gameControlData.dropInTeam;
                    DropTime = gameControlData.dropInTime;
                    Remaining = gameControlData.secsRemaining;
                    SecondaryTime = gameControlData.secondaryTime;

                    // Team Info
                    timNumber1 = gameControlData.teams[0].teamNumber;
                    timNumber2 = gameControlData.teams[1].teamNumber;
                    timColour1 = gameControlData.teams[0].teamColour;
                    timColour2 = gameControlData.teams[1].teamColour;
                    Score1 = gameControlData.teams[0].score;
                    Score2 = gameControlData.teams[1].score;
                    Penaltyshoot1 = gameControlData.teams[0].penaltyShot;
                    Penaltyshoot2 = gameControlData.teams[1].penaltyShot;
                    Singleshoot1 = gameControlData.teams[0].singleShots;
                    Singleshoot2 = gameControlData.teams[1].singleShots;
                    // Coachsequence1	= gameControlData.teams[0].coachSequence;
                    // Coachsequence2	= gameControlData.teams[1].coachSequence;

                    // Robot Info
                    Penalty1 = gameControlData.teams[0].players[Player - 1].penalty;
                    Penalty2 = gameControlData.teams[1].players[Player - 1].penalty;
                    TimeUnpenalis1 = gameControlData.teams[0].players[Player - 1].secsTillUnpenalised;
                    TimeUnpenalis2 = gameControlData.teams[1].players[Player - 1].secsTillUnpenalised;
                    // YellowCard1	= gameControlData.teams[0].players[Player-1].yellowCardCount;
                    // YellowCard2	= gameControlData.teams[1].players[Player-1].yellowCardCount;
                    // RedCard1	= gameControlData.teams[0].players[Player-1].redCardCount;
                    // RedCard2	= gameControlData.teams[1].players[Player-1].redCardCount;

                    //					printf("Game State = %d\n",gameControlData.state);

                    // printf("len = %d\n",len);
                    // printf("data = %s\n",data);
                    // printf("Robocup GameController Data\n\n");
                    // printf("Game State = %d\n",gameControlData.state);
                    // printf("Version = %d\n",gameControlData.version);
                    // printf("packetNumber = %d\n",gameControlData.packetNumber);
                    // printf("playersPerTeam = %d\n",gameControlData.playersPerTeam);
                    // // printf("gameType = %d\n",gameControlData.gameType);
                    // printf("firstHalf = %d\n",gameControlData.firstHalf);
                    // printf("kickOffTeam = %d\n",gameControlData.kickOffTeam);
                    // printf("secondaryState = %d\n",gameControlData.secondaryState);
                    // printf("dropInTeam = %d\n",gameControlData.dropInTeam);
                    // printf("dropInTime = %d\n",gameControlData.dropInTime);
                    // printf("secsRemaining = %d\n",gameControlData.secsRemaining);
                    // printf("secondaryTime = %d\n\n",gameControlData.secondaryTime);

                    // printf("Team Info left\n");
                    // printf("TeamNumber = %d\n",gameControlData.teams[0].teamNumber);
                    // printf("teamColour = %d\n",gameControlData.teams[0].teamColour);
                    // printf("score = %d\n",gameControlData.teams[0].score);
                    // printf("penaltyShot = %d\n",gameControlData.teams[0].penaltyShot);
                    // printf("singleShots = %d\n",gameControlData.teams[0].singleShots);
                    // printf("coachSequence = %d\n\n",gameControlData.teams[0].coachSequence);

                    // printf("Team Info Right\n");
                    // printf("TeamNumber = %d\n",gameControlData.teams[1].teamNumber);
                    // printf("teamColour = %d\n",gameControlData.teams[1].teamColour);
                    // printf("score = %d\n",gameControlData.teams[1].score);
                    // printf("penaltyShot = %d\n",gameControlData.teams[1].penaltyShot);
                    // printf("singleShots = %d\n",gameControlData.teams[1].singleShots);
                    // printf("coachSequence = %d\n\n",gameControlData.teams[1].coachSequence);

                    // printf("\nRobot Info Left\n");
                    // printf("penalty = %d\n",gameControlData.teams[0].players[Player-1].penalty);
                    // printf("secsTillUnpenalised = %d\n",gameControlData.teams[0].players[Player-1].secsTillUnpenalised);
                    // printf("yellowCardCount = %d\n",gameControlData.teams[0].players[Player-1].yellowCardCount);
                    // printf("redCardCount = %d\n\n",gameControlData.teams[0].players[Player-1].redCardCount);

                    // printf("\nRobot Info Right\n");
                    // printf("penalty = %d\n",gameControlData.teams[1].players[Player-1].penalty);
                    // printf("secsTillUnpenalised = %d\n",gameControlData.teams[1].players[Player-1].secsTillUnpenalised);
                    // printf("yellowCardCount = %d\n",gameControlData.teams[1].players[Player-1].yellowCardCount);
                    // printf("redCardCount = %d\n\n",gameControlData.teams[1].players[Player-1].redCardCount);
                }
            }
            len = recvfrom(sock_fd, data, MAX_LENGTH, 0, (struct sockaddr *)&source_addr, &source_addr_len);
            // printf("Game State = %d\n",gameControlData.state);
            // printf("Kick Off = %d\n\n",gameControlData.kickOffTeam);

            auto msg = std_msgs::msg::Int64MultiArray();
            msg.data = {State,
                        FirstHalf,
                        Version,
                        PacketNumber,
                        PlayerTeam,
                        // GameTipe,
                        KickOff,
                        SecondaryState,
                        DropTeam,
                        DropTime,
                        Remaining,
                        SecondaryTime,
                        // ket : 1 = untuk data GameController yang kiri
                        //	 2 = untuk data GameController yang kanan
                        timNumber1,
                        timNumber2,
                        timColour1,
                        timColour2,
                        Score1,
                        Score2,
                        Penaltyshoot1,
                        Penaltyshoot2,
                        Singleshoot1,
                        Singleshoot2,
                        // Coachsequence1,
                        // Coachsequence2,

                        Penalty1,
                        Penalty2,
                        TimeUnpenalis1,
                        TimeUnpenalis2,
                        // YellowCard1,
                        // YellowCard2,
                        // RedCard1,
                        // RedCard2
                        };

            Publisher_->publish(msg);
            // std::cout << "KickOff = " << KickOff << std::endl;
        }
    }

    bool initRet()
    {
        init_return = false;
        if (!init_return)
        {
            sock_rt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            memset((char *)&other, 0, sizeof(other));
            other.sin_family = AF_INET;
            other.sin_port = htons(OUT_PORT);

            //		if (inet_aton(SERVER , &other.sin_addr) == 0) {
            if (inet_aton(Server, &other.sin_addr) == 0)
            {
                fprintf(stderr, "inet_aton() failed\n");
                exit(1);
            }

            strcpy(gameControlReturnData.header, GAMECONTROLLER_RETURN_STRUCT_HEADER);
            gameControlReturnData.version = GAMECONTROLLER_RETURN_STRUCT_VERSION;
            gameControlReturnData.team = Team;
            gameControlReturnData.player = Player;
            // gameControlReturnData.team = TEAM_NUMBER;
            // gameControlReturnData.player = PLAYER_NUMBER;
            gameControlReturnData.message = GAMECONTROLLER_RETURN_MSG_ALIVE;
            memcpy(message, &gameControlReturnData, sizeof(message));

            init_return = true;
        }
        return init_return;
    }

    void returnGameControllerData()
    {
        if (!initRet())
        {
            initRet();
        }

        if (Penalise)
        {
            usleep(500000);
            // sendto(sock_rt, message, strlen(message), 0, (struct sockaddr *)&other, sizeof(other));
            sendto(sock_rt ,"RGrt", 4 , 0 , (struct sockaddr *) &other, sizeof(other));
            // printf("send\n");
        }
    }

private:
    rclcpp::Publisher<std_msgs::msg::Int64MultiArray>::SharedPtr Publisher_;
    rclcpp::Subscription<bfc_msgs::msg::Button>::SharedPtr Subscription_;
    rclcpp::TimerBase::SharedPtr timer_;

    int robotNumber;
    int len, flags, broadcast, sock_fd, nGameControlData;
    int sock_rt;
    bool init_read, init_return;
    double recvTime;
    char data[4096];

    int State, Player, Team,
        FirstHalf,
        Version,
        PacketNumber,
        PlayerTeam,
        GameTipe,
        KickOff,
        SecondaryState,
        DropTeam,
        DropTime,
        Remaining,
        SecondaryTime,
        // ket : 1 = untuk data GameController yang kiri
        //	 2 = untuk data GameController yang kanan
        timNumber1,
        timNumber2,
        timColour1,
        timColour2,
        Score1,
        Score2,
        Penaltyshoot1,
        Penaltyshoot2,
        Singleshoot1,
        Singleshoot2,
        Coachsequence1,
        Coachsequence2,

        Penalty1,
        Penalty2,
        TimeUnpenalis1,
        TimeUnpenalis2,
        YellowCard1,
        YellowCard2,
        RedCard1,
        RedCard2;

    bool Penalise = true;

    const char *Server;
    std::string server_string;
    RoboCupGameControlData gameControlData;
    RoboCupGameControlReturnData gameControlReturnData;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<game_controller>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}