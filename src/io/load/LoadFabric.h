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
        IOCenter *io;

        LoadFabricInterface() = default;

        virtual void deleteHard() = 0;

        virtual void deleteSoft() = 0;

        virtual void resetHard() = 0;

        virtual void resetSoft() = 0;

        virtual ~LoadFabricInterface() = default;
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

        explicit LoadFabric(ABTM::ExecutionType execType, std::string const& rosbridge_port = "localhost:9090", std::string const& ros_client_name = "test") :
        execType(execType), rosbridge_port(rosbridge_port), ros_client_name("test") {
            resetHard();
        }


        void deleteHard() override {
            delete memory;
            deleteSoft();
        }

        void deleteSoft() override {
            delete st;
            delete executor, io, ls, l, vl, nl, tl, rosl, vr, rosc;
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

            st = new SystemTime("time");
            auto f = io->registerIOchannel(st);
            io->registerIOchannel(l);
            io->registerIOchannel(nl);
            io->registerIOchannel(vr);
            io->registerIOchannel(rosc);
            st->start_in_separate_thread(f);
        }

        ~LoadFabric() {
            deleteHard();
        }
    };
}

#endif //ABTM2_LOADFABRIC_H
