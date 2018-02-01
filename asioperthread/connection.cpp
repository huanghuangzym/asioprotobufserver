//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "request_handler.hpp"
#include "logger.h" 
#include "BolService.pb.h"

namespace http {
namespace server2 {

connection::connection(boost::asio::io_service& io_service,
    request_handler& handler,int seqid)
  : socket_(io_service),
    request_handler_(handler),m_seqid(seqid)
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{
  socket_.async_read_some(boost::asio::buffer(buffer_),
      boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
  if (!e)
  {


	
	int packetLen;
	memcpy(&packetLen, buffer_.data(), 4);
	packetLen=ntohl(packetLen);

	//LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] come data length "<<
		//bytes_transferred;
		
	
	 BolServiceResponse response;
        BolServiceRequest  request;

	std::string reqdata(buffer_.data()+4,packetLen);

	 if (request.ParseFromString(reqdata))
	 {
	 	int pid = request.syspid();
		int proid= request.programid();
		
		//LOG_INFO<<"["<<m_seqid<<"] syspid is "<<request.syspid()<<" program id is "<<request.programid();
			//<<"["<<request.DebugString()<<"]";
		
		const BolServiceProto& requestProto = request.proto();                    
                    BolServiceProto *pResponseProto = response.mutable_proto();
                    pResponseProto->set_version(requestProto.version());
                    pResponseProto->set_id(requestProto.id());
                    pResponseProto->set_clientid(requestProto.clientid());

			
			 int requestNum = request.msgs_size();
			  BolServiceResponse_Response *pResrsp = response.add_msgs();
			  BolServiceResponse_UpdateResponse *pUpdateResponse;
                    for (int index = 0; index < requestNum; ++index) 
		    {
                        const BolServiceRequest_Request& rRequestreq = request.msgs(index);
                        switch (rRequestreq.optype()) 
			  {
                        case BolServiceRequest_OperationType_ADD:
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] add request";
                            break;

                        case BolServiceRequest_OperationType_ERASE:
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] erase request";
                            break;

                        case BolServiceRequest_OperationType_QUERY:
                           
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] query request";
                            break;

                        case BolServiceRequest_OperationType_UPDATE:
                            
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] update request";
				pUpdateResponse = pResrsp->mutable_updateresult();
        			pUpdateResponse->set_num(1);
                            break;
                        case BolServiceRequest_OperationType_REPLACE:
                           
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] replace request";
                            break;

                        case BolServiceRequest_OperationType_HEARTBEAT:
                            
                            //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] heatbeat request";
                            break;

                        default:
                            break;
                        }

				
			BolServiceHead *prspHead = pResrsp->mutable_msghead();

			const BolServiceHead& reqhead=rRequestreq.msghead();
			
			    prspHead->set_msgid(reqhead.msgid());
			   pResrsp->set_status(0);
				
                    }

			
			    

					
		      
	 }
		

	m_sendbuffers.clear();
	int sendlen = htonl(response.ByteSize());
	m_sendbuffers.resize(response.ByteSize()+4);
	
	memcpy(&m_sendbuffers[0], &sendlen, 4);
	
	response.SerializeToArray(&m_sendbuffers[0]+4,response.ByteSize());


	   //LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] rsp data total length"
	   		//<<response.ByteSize()+4<<" body len"<<response.ByteSize();
	   		
	
	boost::asio::async_write(socket_, boost::asio::buffer(m_sendbuffers),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
  
    /*boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
    	
      request_handler_.handle_request(request_, reply_);

	  LOG_INFO<<"["<<m_seqid<<"] write conteng length "<<reply_.content.length();
      boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
	  
    }
    else if (!result)
    {
    LOG_INFO<<"["<<m_seqid<<"] bad request ";
      reply_ = reply::stock_reply(reply::bad_request);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
    LOG_INFO<<"["<<m_seqid<<"] need read";
      socket_.async_read_some(boost::asio::buffer(buffer_),
          boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }*/

	
  }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_write(const boost::system::error_code& e)
{

	
	
	

  if (!e)
  {
    // Initiate graceful connection closure.
    
	//LOG_INFO<<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"] write end ,begin to receive ";
		
    socket_.async_read_some(boost::asio::buffer(buffer_),
      boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }
  else
  {
  	//Ð´³ö´í£¬¶ÏÁ´
  	LOG_ERROR <<"["<<m_seqid<<" "<<boost::this_thread::get_id()<<"]write error code "<<e;
  	
  	boost::system::error_code ignored_ec;
    	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.
}

} // namespace server2
} // namespace http
