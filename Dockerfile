FROM debian:bookworm-slim

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		build-essential \
		libreadline-dev \
		make \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["make", "BUILD_DIR=build/docker", "run"]
