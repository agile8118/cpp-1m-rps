#include <drogon/drogon.h>
#include <thread>
#include <string>
#include <vector>

int main() {
    // In the config file we have disabled compression
    drogon::app().loadConfigFile("../config.json");

    // GET /simple route
    drogon::app().registerHandler("/simple",
        [](const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

                // Create the JSON response
                Json::Value ret;
                ret["message"] = "hi";

                auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
                callback(resp);
        },
        { drogon::Get });

    // PATCH /update-something/{id}/{name}
    drogon::app().registerHandler("/update-something/{id}/{name}",
        [](const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id,
            const std::string& name) {

                // Validating
                int idNum = 0;
                try {
                    idNum = std::stoi(id);
                }
                catch (...) {
                    Json::Value ret;
                    ret["error"] = "id must be a number";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(drogon::k400BadRequest);
                    return callback(resp);
                }

                if (name.empty() || name.length() < 3) {
                    Json::Value ret;
                    ret["error"] = "name is required and must be at least 3 characters";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(drogon::k400BadRequest);
                    return callback(resp);
                }

                // Query parameters (value1, value2)
                auto queryParams = req->getParameters();
                std::string v1 = queryParams["value1"];
                std::string v2 = queryParams["value2"];

                // Request body, foo1 to foo10
                auto jsonBody = req->getJsonObject();
                std::string totalFoo = "";

                if (jsonBody) {
                    for (int i = 1; i <= 10; ++i) {
                        std::string key = "foo" + std::to_string(i);
                        if ((*jsonBody).isMember(key)) {
                            if ((*jsonBody)[key].isString()) {
                                totalFoo += (*jsonBody)[key].asString() + ". ";
                            }
                            else {
                                totalFoo += (*jsonBody)[key].asString();
                            }
                        }
                    }
                }

                // Transform totalFoo to Uppercase
                for (auto& c : totalFoo) c = toupper(c);

                // Generate dummy data (around 30 KB)
                Json::Value history(Json::arrayValue);
                for (int i = 0; i < 100; ++i) {
                    Json::Value event;
                    event["event_id"] = idNum + i;
                    event["timestamp"] = trantor::Date::date().toFormattedString(false);
                    event["action"] = "Action performed by " + name;
                    event["metadata"] = "This is a string intended to take up space to simulate a medium-sized production API response object.";
                    event["metadata"] = event["metadata"].asString() + event["metadata"].asString(); // Repeat 2x
                    event["status"] = (i % 2 == 0) ? "success" : "pending";
                    history.append(event);
                }

                // Final response
                Json::Value root;
                root["id"] = id;
                root["name"] = name;
                root["value1"] = v1;
                root["value2"] = v2;
                root["total_foo"] = totalFoo;
                root["history"] = history;

                callback(drogon::HttpResponse::newHttpJsonResponse(root));
        },
        { drogon::Patch });

    // Start the server
    drogon::app().addListener("0.0.0.0", 5555);

    // Automatic thread scaling
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 1;

    drogon::app().setThreadNum(cores);

    LOG_INFO << "Server starting with " << cores << " threads";
    LOG_INFO << "Server running on http://127.0.0.1:5555";

    drogon::app().run();

    return 0;
}