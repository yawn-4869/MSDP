/****************************************************
 *
 * ****     ***     ****    *   *    *****    *****
 * *  *    *   *   *        ****     ***        *
 * *   *    ***     ****    *   *    *****      *
 *
 * interface.cc
 * 2024-04-23 15:28:24
 * Generated by rocket framework rocket_generator.py
 * File will not generate while exist
 * Allow editing
****************************************************/


#include <rocket/common/log.h>
#include <rocket/net/rpc/rpc_closure.h>
#include <rocket/net/rpc/rpc_closure.h>
#include <rocket/net/rpc/rpc_controller.h>
#include <google/protobuf/message.h>

#include "../interface/interface.h"

namespace raft_server {


Interface::Interface(const google::protobuf::Message* req, google::protobuf::Message* rsp, rocket::RpcClosure* done, rocket::RpcController* controller)
  : rocket::RpcInterface(req, rsp, done, controller) {

}

Interface::~Interface() {

}

}