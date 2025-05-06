import matplotlib.pyplot as plt

def read_multi_stream_hex_values(filename):
    with open(filename, 'r') as f:
        streams = []
        current_stream = []
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line == "---------------":
                if current_stream:
                    streams.append(current_stream)
                    current_stream = []
            else:
                current_stream.append(int(line, 16))
        if current_stream:  # Add last stream if not empty
            streams.append(current_stream)
    return streams

def normalize_streams(streams):
    return [[v - stream[0] for v in stream] for stream in streams]

def plot_multi_streams(streams, output_pdf='hex_multistream_plot.pdf'):
    colors = plt.cm.get_cmap('tab10', len(streams))  # Get distinct colors
    plt.figure(figsize=(12, 6))

    for i, stream in enumerate(streams):
        x = list(range(len(stream)))
        y = stream
        plt.scatter(x, y, color=colors(i), s=20, label=f'Stream {i+1}')

    plt.title('Normalized Hex Streams Over Time')
    plt.xlabel('Time Step')
    plt.ylabel('Normalized Value')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_pdf)
    print(f"Plot saved as {output_pdf}")

# Replace 'your_file.txt' with the path to your file
file_path = 'TESTS-REF.log'
streams = read_multi_stream_hex_values(file_path)
normalized_streams = normalize_streams(streams)
plot_multi_streams(normalized_streams, output_pdf='TESTS-REF_plot.pdf')

# Replace 'your_file.txt' with the path to your file
file_path = 'TESTS-OPT.log'
streams = read_multi_stream_hex_values(file_path)
normalized_streams = normalize_streams(streams)
plot_multi_streams(normalized_streams, output_pdf='TESTS-OPT_plot.pdf')