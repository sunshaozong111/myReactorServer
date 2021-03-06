#include "HttpServer.h"
#include "../../net/Logging.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

void defaultHttpCallback(const HttpRequest& reqt, HttpResponse* resp){
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name):
    server_(loop, listenAddr, name),
    httpCallback_(defaultHttpCallback){
        server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

HttpServer::~HttpServer(){

}

void HttpServer::start(){
    LOG_INFO << "HttpServer strat listenning.";
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn){
    if(conn->connected()){
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime){
    HttpContext* context = &((*conn->getNonConstContext()).anyCast<HttpContext>());
    if(!context->parseRequest(buf, receiveTime)){
        conn->send("Http/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
    if(context->isGotAll()){
        onRequest(conn, context->getRequest());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req){
    const std::string& connection = req.getAHeader("Connection");
    bool close = (connection == "close" || (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive"));
    HttpResponse response(close);
    // provided by user
    httpCallback_(req, &response);
    Buffer buf;
    // write the response message 
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if(response.getCloseConnection()){
        conn->shutdown();
    }
}



