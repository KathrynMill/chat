-- 創建資料庫（如果不存在）
CREATE DATABASE IF NOT EXISTS chatdb CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE chatdb;

-- 用戶表
CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB;

-- 好友關係表
CREATE TABLE IF NOT EXISTS friends (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    status ENUM('pending', 'accepted', 'blocked') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_friendship (user_id, friend_id),
    INDEX idx_user_id (user_id),
    INDEX idx_friend_id (friend_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 群組表
CREATE TABLE IF NOT EXISTS groups (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    owner_id INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_owner_id (owner_id),
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 群組成員表
CREATE TABLE IF NOT EXISTS group_members (
    id INT PRIMARY KEY AUTO_INCREMENT,
    group_id INT NOT NULL,
    user_id INT NOT NULL,
    role ENUM('owner', 'admin', 'member') DEFAULT 'member',
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_membership (group_id, user_id),
    INDEX idx_group_id (group_id),
    INDEX idx_user_id (user_id),
    FOREIGN KEY (group_id) REFERENCES groups(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 訊息表
CREATE TABLE IF NOT EXISTS messages (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    from_id INT NOT NULL,
    to_id INT,
    group_id INT,
    content TEXT NOT NULL,
    message_type ENUM('text', 'image', 'file') DEFAULT 'text',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_from_id (from_id),
    INDEX idx_to_id (to_id),
    INDEX idx_group_id (group_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (from_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES groups(id) ON DELETE CASCADE,
    CHECK ((to_id IS NOT NULL AND group_id IS NULL) OR (to_id IS NULL AND group_id IS NOT NULL))
) ENGINE=InnoDB;

-- 離線訊息表
CREATE TABLE IF NOT EXISTS offline_msgs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    from_id INT NOT NULL,
    content TEXT NOT NULL,
    message_type ENUM('text', 'image', 'file') DEFAULT 'text',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_from_id (from_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (from_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 插入測試數據
INSERT INTO users (username, password_hash, email) VALUES
('alice', '$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewdBPj4J/8Kz8Kz2', 'alice@example.com'),
('bob', '$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewdBPj4J/8Kz8Kz2', 'bob@example.com'),
('charlie', '$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewdBPj4J/8Kz8Kz2', 'charlie@example.com');

-- 插入測試好友關係
INSERT INTO friends (user_id, friend_id, status) VALUES
(1, 2, 'accepted'),
(2, 1, 'accepted'),
(1, 3, 'accepted'),
(3, 1, 'accepted');

-- 插入測試群組
INSERT INTO groups (name, description, owner_id) VALUES
('Test Group', 'A test group for development', 1);

-- 插入測試群組成員
INSERT INTO group_members (group_id, user_id, role) VALUES
(1, 1, 'owner'),
(1, 2, 'member'),
(1, 3, 'member');
