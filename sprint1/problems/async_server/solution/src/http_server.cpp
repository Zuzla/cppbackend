#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {    
    
    void ReportError(beast::error_code ec, std::string_view what) 
    {
        std::cerr << what << ": " << ec.message() << std::endl;
    }

    void SessionBase::Run() {
        // Вызываем метод Read, используя executor объекта stream_.
        // Таким образом вся работа со stream_ будет выполняться, используя его executor
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }








}  // namespace http_server
