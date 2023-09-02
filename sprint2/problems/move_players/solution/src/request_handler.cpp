#include "request_handler.h"

namespace http_handler 
{
    using namespace std::literals;

    std::string RequestHandler::Error(std::string code, std::string msg)
    {
        code.erase(std::remove_if(code.begin(), code.end(), ::isspace), code.end());
        code[0] = tolower(code[0]);
        
        boost::json::object error = 
        {
            {"code", code},
            {"message", msg}
        };

        return json::serialize(error);
    }    

    RequestHandler::StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);        
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler::FileResponse RequestHandler::MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, bool keep_alive, std::string_view content_type) 
    {
        FileResponse response;
        response.version(http_version);
        response.result(status);
        response.insert(http::field::content_type, content_type);        
        response.body() = std::move(body);
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        response.prepare_payload();
        return response;
    }

    std::string_view RequestHandler::GetContenType(fs::path&& path)
    {
        try
        {
            return map_extension.at(path.extension().string());
        }
        catch(const std::exception& e)
        {
            return ContentType::TEXT_HTML;
        }   
    }    

    bool RequestHandler::IsSubPath(fs::path&& path, fs::path base) 
    {
        
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(game_file_path_ / path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    RequestHandler::FileRequestResult RequestHandler::RequestFile(const StringRequest& req, fs::path path)
    {
        if (!path.has_extension())
        {
            return (MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_HTML));
        }      

        if (!IsSubPath(std::move(path), game_file_path_))
        {
            return (MakeStringResponse(http::status::not_found, "File not found!", req.version(), req.keep_alive(), ContentType::TEXT_TXT));
        }

        http::file_body::value_type file;

        if (sys::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec) 
        {
            return (MakeStringResponse(http::status::not_found, "File not found!", req.version(), req.keep_alive(), ContentType::TEXT_TXT));
        }        

        return (MakeFileResponse(http::status::ok, std::move(file), req.version(), req.keep_alive(), GetContenType(std::move(path))));
    }

    RequestHandler::StringResponse RequestHandler::ReportServerError(const StringRequest& req)
    {
        return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_HTML);
    }
}  // namespace http_handler
