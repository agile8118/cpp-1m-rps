#include <drogon/drogon.h>
#include <thread>
#include <string>
#include <vector>
#include <charconv>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

int main() {
    // In the config file we have disabled compression and logging
    drogon::app().loadConfigFile("../config.json");

    // GET /simple route
    drogon::app().registerHandler("/simple",
        [](const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

                rapidjson::StringBuffer sb;
                rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

                writer.StartObject();

                writer.Key("message");
                writer.String("hi");

                writer.EndObject();

                // Create a standard http response
                auto res = drogon::HttpResponse::newHttpResponse();

                // Set content type to JSON
                res->setContentTypeCode(drogon::CT_APPLICATION_JSON);

                // Move the string buffer into the response body
                res->setBody(std::string(sb.GetString(), sb.GetSize()));

                callback(res);
        },
        { drogon::Get });



    // PATCH /update-something/{id}/{name} route
    drogon::app().registerHandler("/update-something/{id}/{name}",
        [](const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id,
            const std::string& name) {

                // Validation
                int idNum = 0;
                try {
                    idNum = std::stoi(id);
                }
                catch (...) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setBody(R"({"error":"id must be a number"})");
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    return callback(resp);
                }

                if (name.length() < 3) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setBody(R"({"error":"name is required and must be at least 3 characters"})");
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    return callback(resp);
                }

                // Query parameters
                auto queryParams = req->getParameters();
                const std::string& v1 = queryParams["value1"];
                const std::string& v2 = queryParams["value2"];

                std::string totalFoo = "";
                std::string_view rawBody = req->body();
                rapidjson::Document doc;

                static const std::vector<std::string> fooKeys = [] {
                    std::vector<std::string> keys;
                    for (int i = 1; i <= 10; ++i) keys.push_back("foo" + std::to_string(i));
                    return keys;
                    }();

                // Parse the raw string directly with RapidJSON
                if (!doc.Parse(rawBody.data(), rawBody.size()).HasParseError() && doc.IsObject()) {
                    for (const auto& key : fooKeys) {
                        if (doc.HasMember(key.c_str())) {
                            const auto& val = doc[key.c_str()];
                            if (val.IsString()) {
                                totalFoo += val.GetString();
                                totalFoo += ". ";
                            }
                            else if (!val.IsNull()) {
                                totalFoo += "NON_STRING_VAL";
                            }
                        }
                    }
                }

                for (auto& c : totalFoo) c = toupper(static_cast<unsigned char>(c));

                // Serialization with RapidJSON
                rapidjson::StringBuffer sb;
                rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

                writer.StartObject();

                writer.Key("id");
                writer.String(id.c_str());

                writer.Key("name");
                writer.String(name.c_str());

                writer.Key("value1");
                writer.String(v1.c_str());

                writer.Key("value2");
                writer.String(v2.c_str());

                writer.Key("total_foo");
                writer.String(totalFoo.c_str());


                writer.Key("history");
                writer.StartArray();

                std::string timestamp = trantor::Date::date().toFormattedString(false);
                std::string action = "Action performed by " + name;
                std::string metadata = "This is a string intended to take up space to simulate a medium-sized production API response object.";
                metadata += metadata;

                for (int i = 0; i < 100; ++i) {
                    writer.StartObject();
                    writer.Key("event_id");
                    writer.Int(idNum + i);

                    writer.Key("timestamp");
                    writer.String(timestamp.c_str());

                    writer.Key("action");
                    writer.String(action.c_str());

                    writer.Key("metadata");
                    writer.String(metadata.c_str());

                    writer.Key("status");
                    writer.String((i % 2 == 0) ? "success" : "pending");
                    writer.EndObject();
                }
                writer.EndArray();
                writer.EndObject();

                // Final response
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setBody(std::string(sb.GetString(), sb.GetSize()));
                res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                callback(res);
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