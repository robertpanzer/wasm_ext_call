This compiles an example filter for envoy WASM.

# build filter
(The latest binary is at ./filter.wasm though)

build with
```
bazel build :filter.wasm
```

Filter will be in:
```
./bazel-bin/filter.wasm
```

# run 

Copy filter:

```
cp bazel-bin/filter.wasm
```

Run istio-proxy with the filter

```
docker run -ti --rm -p 8888:8888 -v $(pwd)/host --entrypoint envoy istio/proxyv2:1.7.4 -c /host/envoy.yaml
```



Send request a couple of times, with payload:
```
curl localhost:8888 --data 'kfjdshfkjdhfksdjhfkdsjhfdskjhfdskjhfdskjhfdskjfhdskjfhdskjfhkfjhdskjfhdskjfhdskjfh'
```

At some point the context is deleted before the external request has been processed.

```
[2020-11-18 13:05:36.691][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:49]::onRequestHeaders() onRequestHeaders 3
[2020-11-18 13:05:36.695][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:80]::onResponseHeaders() onResponseHeaders 3
[2020-11-18 13:05:36.696][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:90]::onDone() onDone 3
[2020-11-18 13:05:36.697][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:92]::onLog() onLog 3
[2020-11-18 13:05:36.698][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:94]::onDelete() onDelete 3
[2020-11-18 13:05:36.701][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:59]::operator()() GOT_RESPONSE
[2020-11-18 13:05:36.702][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:61]::operator()() CONTEXT_ID IN RESPONSE HANDLER: 32
[2020-11-18 13:05:36.703][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:64]::operator()() RESPONSE PAYLOAD IN HEADERS: { "Hello": "World" }

[2020-11-18 13:05:36.703][23][info][wasm] [external/envoy/source/extensions/common/wasm/context.cc:1009] wasm log: [filter.cc:71]::operator()() RESPONSE RESULT 0
```

Also the response comes from the original route, not from the external request:
```
# curl localhost:8888 --data 'kfjdshfkjdhfksdjhfkdsjhfdskjhfdskjhfdskjhfdskjfhdskjfhdskjfhkfjhdskjfhdskjfhdskjfh'
{ "Hello": "World" }
# curl localhost:8888 --data 'kfjdshfkjdhfksdjhfkdsjhfdskjhfdskjhfdskjhfdskjfhdskjfhdskjfhkfjhdskjfhdskjfhdskjfh'
example body
```