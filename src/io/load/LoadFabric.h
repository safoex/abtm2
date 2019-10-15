//
// Created by safoex on 07.10.19.
//

#ifndef ABTM2_LOADFABRIC_H
#define ABTM2_LOADFABRIC_H

#include "Loaders.h"
#include "io/ros/ROSCommandsConverter.h"
#include "io/addons/time/SystemTime.h"


namespace abtm {

    class LoadFabricInterface {
    public:
        IOCenter *io, *old_io;

        LoadFabricInterface() = default;

        virtual void deleteHard() = 0;

        virtual void deleteSoft() = 0;

        virtual void resetHard() = 0;

        virtual void resetSoft() = 0;

        virtual ~LoadFabricInterface() = default;

        virtual std::string get_memory() = 0;
    };

    template<class VarType, class MemType>
    class LoadFabric : public LoadFabricInterface {
    public:
        MemType *memory;
        ABTM::ExecutionType execType;
        ABTM* executor;
        Loaders *ls;
        Loader *l;
        VarsLoader<VarType> *vl;
        NodesLoader *nl;
        TemplatesLoader *tl;
        ROSLoader *rosl;
        ROSCommandsConverter *rosc;
        ViewRep *vr;
        SystemTime* st;
        std::string rosbridge_port;
        std::string ros_client_name;
        bool initialized;


        explicit LoadFabric(ABTM::ExecutionType execType, std::string const& rosbridge_port = "localhost:9090", std::string const& ros_client_name = "test") :
        execType(execType), rosbridge_port(rosbridge_port), ros_client_name("test") {
            old_io = nullptr;
            resetHard();
        }


        void deleteHard() override {
            delete memory;
            deleteSoft();
        }

        void deleteSoft() override {
            io->stop();
            if(old_io)
                delete old_io;
            // LEAK!!
            // NOW NO LEAK BUT WEIRD AS F@CK
            old_io = io;
            st->stop();
            delete st;
            std::cout << "delete st;" << std::endl;
            delete rosl;
            std::cout << "delete rosl;" << std::endl;
            delete executor;
            std::cout << "delete ex;" << std::endl;
            delete ls;
            std::cout << "delete ls;" << std::endl;
            delete l;
            delete vl;
            delete nl;
            delete tl;
            delete vr;
            delete rosc;
            std::cout << "delete rest;" << std::endl;
            initialized = false;
            executor = nullptr;
        }

        void resetHard() override {
            memory = new MemType;
            resetSoft();
        }

        void resetSoft() override {
            executor = new ABTM(execType, memory);
            io = new IOCenter(executor);
            ls = new Loaders(executor);
            l  = new Loader(ls);
            vl = new VarsLoader<VarType>(ls);
            nl = new NodesLoader(ls);
            tl = new TemplatesLoader(ls);
            rosl = new ROSLoader(ls, io, ros_client_name, rosbridge_port);
            rosc = new ROSCommandsConverter();
            vr = new ViewRep(executor);
            ls->add_loader(l->WORD(), l);
            ls->add_loader(vl->WORD(), vl);
            ls->add_loader(tl->WORD(), tl, "", vl->WORD());
            ls->add_loader(nl->WORD(),nl, "", tl->WORD());
            ls->add_loader(rosl->WORD(), rosl, "", nl->WORD());
            ls->executor = executor;

            st = new SystemTime("sys_time", 1);
            auto f = io->registerIOchannel(st);
            st->start_in_separate_thread(f);
            io->registerIOchannel(l);
            io->registerIOchannel(nl);
            io->registerIOchannel(vr);
            io->registerIOchannel(rosc);
            initialized = true;
        }

        std::string get_memory() override {
            return "";
        }

        ~LoadFabric() {
            deleteHard();
        }
    };

    template<>
    std::string LoadFabric<std::string, MemoryDuktape>::get_memory() {
        return memory->dump_memory();
    }
}

#endif //ABTM2_LOADFABRIC_H
