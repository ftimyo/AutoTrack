#include <boost/asio.hpp>
#include <memory>
template <typename T>
struct TCPServerSession : public std::enable_shared_from_this<TCPServerSession<T>> {
	using tcp = boost::asio::ip::tcp;
	using Session = TCPServerSession<T>;
	using SessionPtr = std::shared_ptr<Session>;
	template <typename... Ts>
	static auto makeSession(Ts&&... params) {
		return SessionPtr{new Session{std::forward<Ts>(params)...}};
	}
	void AsyncOneTimeRead(T& msg) {
		auto self{this->shared_from_this()};
		boost::asio::async_read(sk_,boost::asio::buffer(&msg,sizeof(msg)),
				[self,this](auto, auto){
				});
	}
private:
	tcp::socket sk_;
	TCPServerSession(tcp::socket&& sk) : sk_{std::move(sk)}{}
};
template <typename T>
struct TCPServer : public std::enable_shared_from_this<TCPServer<T>> {
public:
	using tcp = boost::asio::ip::tcp;
	using SRV = TCPServer<T>;
	using SRVPtr = std::shared_ptr<SRV>;
	template <typename... Ts>
	static auto makeSRV(Ts&&... params) {
		return SRVPtr{new SRV{std::forward<Ts>(params)...}};
	}
	boost::asio::io_service& GetIOService() {
		return ask_.get_io_service();
	}

	void AcceptOneTimeSession(T& msg) {
		auto self{this->shared_from_this()};
		ask_.async_accept(sk_,[&msg=msg,self,this](auto ec){
					if (ec) return;
					auto session = TCPServerSession<T>::makeSession(std::move(sk_));
					session->AsyncOneTimeRead(msg);
					this->AcceptOneTimeSession(msg);
				});
	}
private:
	tcp::acceptor ask_;
	tcp::socket sk_;
	TCPServer(boost::asio::io_service& io_service, int port):
		ask_{io_service, tcp::endpoint(tcp::v4(),port)},
		sk_{io_service}{}
};
