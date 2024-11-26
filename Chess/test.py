if __name__ == '__main__':
    import psutil
    process = psutil.Process()
    print(process.memory_info().rss / 1024  / 1024)