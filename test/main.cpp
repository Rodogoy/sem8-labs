#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <string>
#include <vector>
#include <thread>
#include "json.hpp"
#include "httplib.h"

using json = nlohmann::json;

struct HashCrackRequest { std::string hash; int maxLength = 3; NLOHMANN_DEFINE_TYPE_INTRUSIVE(HashCrackRequest, hash, maxLength)};
struct HashCrackResponse { std::string requestId; NLOHMANN_DEFINE_TYPE_INTRUSIVE(HashCrackResponse, requestId)};
struct CrackStatus { std::string status; std::vector<std::string> data; NLOHMANN_DEFINE_TYPE_INTRUSIVE(CrackStatus, status, data)};


char hashbuf[256] = { };
char maxlengthbuf[2] = { };
std::string statusMessage = "Enter the hash and press 'Send'";
std::vector<std::string> results;
bool isRequestInProgress = false;
float progress = 0.0f;

void sendCrackRequest() {
    isRequestInProgress = true;
    statusMessage = "Sending a request...";

    httplib::Client client("http://localhost:8080");
    std::string inputHash = std::string(hashbuf);
    HashCrackRequest request{ std::string(hashbuf), maxlengthbuf[0]};
    json reqJson = { { "hash", std::string(hashbuf) },
                     { "maxLength", (int)(maxlengthbuf[0] - '0') } };

    auto res = client.Post("/api/hash/crack", reqJson.dump(), "application/json");
    if (!res || res->status != 200) {
        statusMessage = "Request error!";
        isRequestInProgress = false;
        return;
    }

    try {
        auto jsonId = json::parse(res->body);
        std::string requestId = jsonId["requestId"];
        statusMessage = "Request Submitted. ID: " + requestId;

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            auto statusRes = client.Post("/api/hash/status", jsonId.dump(), "application/json");

            if (!statusRes) {
                statusMessage = "Status Check Error! ";
                break;
            }

            auto jsonStatus = json::parse(statusRes->body);
            std::string status = jsonStatus["status"];
            std::vector<std::string> Data = jsonStatus["data"];
            if (status == "IN_PROGRESS") {
                if (!Data.empty()) {
                    progress = std::stof(Data[0]); 
                }
                statusMessage = "In Progress...";
            }
            else if (status == "READY") {
                results = Data;
                statusMessage = "Done! Results found: " + std::to_string(results.size());
                break;
            }
            else if (status == "PART_READY") {
                results = Data;
                statusMessage = "Partially done! There are some tasks not completed ";
                break;
            }
            else if (status == "FAIL") {
                statusMessage = "Failed to match the hash!";
                break;
            }
        }
    }
    catch (...) {
        statusMessage = "Response processing error!";
    }

    isRequestInProgress = false;
}

int main() {
    ImGuiContext* my_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(my_context);

    sf::RenderWindow window(sf::VideoMode( 800, 600 ), "Hash Cracker");
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    sf::Clock deltaClock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Hash Cracker");

        ImGui::InputTextWithHint("##hash", "Enter the MD5 hash...", hashbuf, sizeof(hashbuf));
        ImGui::InputTextWithHint("##count", "Enter the maximum search length for the result...", maxlengthbuf, sizeof(maxlengthbuf));

        std::string inputHash = std::string(hashbuf);
        if (ImGui::Button("Send", ImVec2(100, 30)) && !isRequestInProgress) 
            if (std::string(hashbuf).empty()) 
                statusMessage = "Error: hash cannot be empty!";
            else 
                if (std::string(maxlengthbuf).empty()) 
                    statusMessage = "Error: the maximum length of the result search cannot be empty!";
                else
                    if (!std::isdigit(maxlengthbuf[0]))
                        statusMessage = "Error: the maximum search length of the result must be a single digit!";
                    else
                        std::thread(sendCrackRequest).detach();


        if (isRequestInProgress) 
            ImGui::ProgressBar(progress / 100.0f, ImVec2(-1, 20));

        ImGui::Text("%s", statusMessage.c_str());

        if (!results.empty()) {
            ImGui::Separator();
            ImGui::Text("Results found:");
            for (const auto& res : results) {
                ImGui::BulletText("%s", res.c_str());
            }
        }

        ImGui::End();

        window.clear(sf::Color(20, 20, 20));
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::DestroyContext();
    ImGui::SFML::Shutdown();
    return 0;
}