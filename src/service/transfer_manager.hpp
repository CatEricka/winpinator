#pragma once
#include "observable_service.hpp"
#include "remote_manager.hpp"
#include "transfer_types.hpp"
#include "zlib_deflate.hpp"

#include <wx/wx.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace srv
{

typedef std::shared_ptr<TransferOp> TransferOpPtr;

class RemoteManager;

class TransferManager
{
public:
    explicit TransferManager( ObservableService* service );
    ~TransferManager();

    void setOutputPath( const std::wstring& outputPath );
    std::wstring getOutputPath();

    void setRemoteManager( std::shared_ptr<RemoteManager> ptr );
    RemoteManager* getRemoteManager();

    void setCompressionLevel( int level );
    int getCompressionLevel();

    void stop();

    void registerTransfer( const std::string& remoteId, 
        TransferOp& transfer, bool firstTry );

    void replyAllowTransfer( const std::string& remoteId,
        int transferId, bool allow );
    void cancelTransferRequest( const std::string& remoteId,
        time_t timestamp );
    void resumeTransfer( const std::string& remoteId, int transferId );
    void pauseTransfer( const std::string& remoteId, int transferId );
    void stopTransfer( const std::string& remoteId, int transferId );
    void finishTransfer( const std::string& remoteId, int transferId );
    void failAll( const std::string& remoteId );

    std::mutex& getMutex();

    std::vector<TransferOpPtr> getTransfersForRemote( 
        const std::string& remoteId );

private:
    class StartTransferReactor 
        : public grpc::experimental::ClientReadReactor<FileChunk>
    {
    public:
        void setInstance( std::shared_ptr<StartTransferReactor> selfPtr );
        void setRefs( std::shared_ptr<grpc::ClientContext> ref1,
            std::shared_ptr<OpInfo> ref2 );
        void setTransferPtr( TransferOpPtr transferPtr );
        void setRemoteId( const std::string& remoteId );
        void setManager( TransferManager* mgr );

        void start();

        void OnDone( const grpc::Status& s ) override;
        void OnReadDone( bool ok ) override;
    private:
        std::shared_ptr<StartTransferReactor> m_selfPtr;

        // gRPC ref holders - they will be released after this reader object
        // deletes itself
        std::shared_ptr<grpc::ClientContext> m_clientCtx;
        std::shared_ptr<OpInfo> m_request;

        TransferOpPtr m_transfer;
        std::string m_remoteId;
        TransferManager* m_mgr;

        FileChunk m_chunk;
        ZlibDeflate m_compressor;
        bool m_useCompression;

        wxString getAbsolutePath( wxString relativePath );
        void updatePaths();
        void updateProgress( long long chunkBytes );
    };

    static const long long PROGRESS_FREQ_MILLIS;

    std::atomic_bool m_running;
    std::shared_ptr<RemoteManager> m_remoteMgr;

    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;
    std::wstring m_outputPath;
    int m_compressionLevel;
    TransferOp m_empty;

    std::map<std::string, std::vector<TransferOpPtr>> m_transfers;

    void checkTransferDiskSpace( TransferOp& op );
    void checkTransferMustOverwrite( TransferOp& op );
    void setTransferTimestamp( TransferOp& op );
    void setUpPauseLock( TransferOp& op );
    void sendNotifications( const std::string& remoteId, TransferOp& op );

    TransferOpPtr getTransferInfo( const std::string& remoteId, int transferId );
    TransferOpPtr getTransferByTimestamp(
        const std::string& remoteId, time_t timestamp );

    void processStartTransfer( const std::string& remoteId, TransferOpPtr op );
    void processDeclineTransfer( const std::string& remoteId, TransferOpPtr op );

    OpInfo convertOpToOpInfo( const TransferOpPtr op, 
        bool compressionEnabled ) const;
    void sendStatusUpdateNotification( const std::string& remoteId, 
        const TransferOpPtr op );
};

};
